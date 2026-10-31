#include "Referee.hpp"

#include <cstdio>
#include <ctime>
#include <string>

namespace {

pthread_mutex_t g_print_mutex = PTHREAD_MUTEX_INITIALIZER;
constexpr int BOARD_PRINT_EVERY = 20;
constexpr long WAKE_INTERVAL_NS = 50L * 1000000L;
constexpr int EVENT_MOVE = 0;
constexpr int EVENT_CAPTURE = 1;

}

Referee::Referee(Board& board, const MatchArgs& args)
    : board_(board),
      match_id_(args.match_id),
      table_id_(args.table_id),
      puma_player_(args.player_puma_id),
      dogs_player_(args.player_dogs_id),
      abort_flag_(args.global_abort),
      stop_(false),
      decided_(false),
      result_(MatchResult::MATCH_ABORTED_SIGINT),
      captures_(0),
      moves_(0),
      judge_(),
      judge_started_(false) {
    pthread_mutex_init(&mx_, nullptr);
    pthread_cond_init(&cv_, nullptr);
}

Referee::~Referee() {
    join();
    pthread_cond_destroy(&cv_);
    pthread_mutex_destroy(&mx_);
}

bool Referee::start() {
    if (puma_trapped()) {
        declare(MatchResult::WINNER_DOGS);
    }
    if (pthread_create(&judge_, nullptr, &Referee::judge_entry, this) != 0) {
        return false;
    }
    judge_started_ = true;
    return true;
}

void Referee::join() {
    if (judge_started_) {
        pthread_join(judge_, nullptr);
        judge_started_ = false;
    }
}

void* Referee::judge_entry(void* arg) {
    static_cast<Referee*>(arg)->judge_loop();
    return nullptr;
}

bool Referee::aborted() const {
    return abort_flag_ != nullptr && abort_flag_->load();
}

bool Referee::should_stop() const {
    return stop_.load() || aborted();
}

void Referee::decide_locked(MatchResult r) {
    if (!decided_) {
        decided_ = true;
        result_ = r;
        stop_.store(true);
    }
    pthread_cond_broadcast(&cv_);
}

void Referee::declare(MatchResult r) {
    pthread_mutex_lock(&mx_);
    decide_locked(r);
    pthread_mutex_unlock(&mx_);
}

MatchResult Referee::result() {
    pthread_mutex_lock(&mx_);
    MatchResult r = result_;
    pthread_mutex_unlock(&mx_);
    return r;
}

void Referee::push_event(const Event& e) {
    pthread_mutex_lock(&mx_);
    queue_.push_back(e);
    pthread_cond_signal(&cv_);
    pthread_mutex_unlock(&mx_);
}

bool Referee::request_move(int piece, int from, int to) {
    if (should_stop()) {
        return false;
    }
    if (from < 0 || from >= KOMIKAN_NODES || to < 0 || to >= KOMIKAN_NODES) {
        return false;
    }
    if (!board_.adjacent(from, to)) {
        return false;
    }
    std::vector<int> nodes{from, to};
    board_.lock_nodes(nodes);
    bool ok = board_.owner(from) == piece && board_.owner(to) == CELL_EMPTY;
    if (ok) {
        board_.set_owner(to, piece);
        board_.set_owner(from, CELL_EMPTY);
        push_event({EVENT_MOVE, piece, from, to, -1, -1, captures_.load()});
    }
    board_.unlock_nodes(nodes);
    return ok;
}

bool Referee::request_jump(int from, int over, int land) {
    if (should_stop()) {
        return false;
    }
    if (from < 0 || from >= KOMIKAN_NODES || over < 0 || over >= KOMIKAN_NODES ||
        land < 0 || land >= KOMIKAN_NODES) {
        return false;
    }
    if (!board_.valid_jump(from, over, land)) {
        return false;
    }
    std::vector<int> nodes{from, over, land};
    board_.lock_nodes(nodes);
    int victim = board_.owner(over);
    bool ok = board_.owner(from) == PIECE_PUMA && victim >= 1 && board_.owner(land) == CELL_EMPTY;
    if (ok) {
        board_.set_owner(over, CELL_EMPTY);
        board_.set_owner(from, CELL_EMPTY);
        board_.set_owner(land, PIECE_PUMA);
        int total = captures_.fetch_add(1) + 1;
        push_event({EVENT_CAPTURE, PIECE_PUMA, from, land, over, victim, total});
    }
    board_.unlock_nodes(nodes);
    return ok;
}

bool Referee::puma_trapped() {
    for (int attempt = 0; attempt < 8; ++attempt) {
        int pos = board_.puma_position();
        if (pos < 0) {
            return false;
        }
        std::vector<int> nodes{pos};
        for (int n : board_.neighbors(pos)) {
            nodes.push_back(n);
        }
        for (const Jump& j : board_.jumps(pos)) {
            nodes.push_back(j.over);
            nodes.push_back(j.land);
        }
        board_.lock_nodes(nodes);
        if (board_.puma_position() != pos) {
            board_.unlock_nodes(nodes);
            continue;
        }
        bool free_move = false;
        for (int n : board_.neighbors(pos)) {
            if (board_.owner(n) == CELL_EMPTY) {
                free_move = true;
            }
        }
        for (const Jump& j : board_.jumps(pos)) {
            if (board_.owner(j.over) >= 1 && board_.owner(j.land) == CELL_EMPTY) {
                free_move = true;
            }
        }
        board_.unlock_nodes(nodes);
        return !free_move;
    }
    return false;
}

void Referee::log(const std::string& text) {
    char prefix[64];
    std::snprintf(prefix, sizeof prefix, "[Mesa %d | Partida %d] ", table_id_, match_id_);
    pthread_mutex_lock(&g_print_mutex);
    std::fputs(prefix, stdout);
    std::fputs(text.c_str(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
    pthread_mutex_unlock(&g_print_mutex);
}

std::string Referee::describe(const Event& e) const {
    char buf[160];
    if (e.kind == EVENT_CAPTURE) {
        std::snprintf(buf, sizeof buf,
                      "Puma salta de %d a %d y captura al Perro %d (casilla %d) | capturas: %d/%d",
                      e.from, e.to, e.victim, e.over, e.total, KOMIKAN_PUMA_WIN);
    } else if (e.piece == PIECE_PUMA) {
        std::snprintf(buf, sizeof buf, "Puma mueve %d -> %d", e.from, e.to);
    } else {
        std::snprintf(buf, sizeof buf, "Perro %d mueve %d -> %d", e.piece, e.from, e.to);
    }
    return buf;
}

const char* Referee::describe(MatchResult r) const {
    switch (r) {
        case MatchResult::WINNER_PUMA:
            return "Gana el Puma (capturo 6 o mas perros)";
        case MatchResult::WINNER_DOGS:
            return "Ganan los Perros (Puma acorralado)";
        case MatchResult::ABANDON_PUMA:
            return "Abandona el jugador Puma: ganan los Perros por abandono";
        case MatchResult::ABANDON_DOGS:
            return "Abandona el jugador Perros: gana el Puma por abandono";
        case MatchResult::MATCH_ABORTED_SIGINT:
            return "Partida abortada por SIGINT";
    }
    return "Resultado desconocido";
}

void Referee::judge_loop() {
    char head[160];
    std::snprintf(head, sizeof head, "Inicio: Puma (jugador %d) vs Perros (jugador %d)",
                  puma_player_, dogs_player_);
    log(std::string(head) + "\n" + board_.render());

    while (true) {
        std::vector<Event> batch;
        pthread_mutex_lock(&mx_);
        while (queue_.empty() && !decided_ && !aborted()) {
            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += WAKE_INTERVAL_NS;
            if (ts.tv_nsec >= 1000000000L) {
                ts.tv_sec += 1;
                ts.tv_nsec -= 1000000000L;
            }
            pthread_cond_timedwait(&cv_, &mx_, &ts);
        }
        if (!decided_ && aborted()) {
            decide_locked(MatchResult::MATCH_ABORTED_SIGINT);
        }
        batch.swap(queue_);
        bool done = decided_;
        pthread_mutex_unlock(&mx_);

        bool capture_seen = false;
        for (const Event& e : batch) {
            ++moves_;
            if (e.kind == EVENT_CAPTURE) {
                capture_seen = true;
            }
            log(describe(e));
        }
        if (!batch.empty() && (capture_seen || moves_ % BOARD_PRINT_EVERY < static_cast<int>(batch.size()))) {
            log("Estado del tablero tras " + std::to_string(moves_) + " movimientos\n" + board_.render());
        }

        if (!done) {
            if (captures_.load() >= KOMIKAN_PUMA_WIN) {
                declare(MatchResult::WINNER_PUMA);
            } else if (puma_trapped()) {
                declare(MatchResult::WINNER_DOGS);
            }
        }

        pthread_mutex_lock(&mx_);
        done = decided_;
        pthread_mutex_unlock(&mx_);
        if (done) {
            break;
        }
    }

    MatchResult r = result();
    log(std::string("Resultado final: ") + describe(r) + " | capturas: " +
        std::to_string(captures_.load()) + " | movimientos: " + std::to_string(moves_) + "\n" +
        board_.render());
}