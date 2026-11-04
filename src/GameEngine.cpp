#include "GameEngine.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <pthread.h>
#include <unistd.h>

namespace {

constexpr int SLEEP_SLICE_MS = 20;
constexpr int DOG_CHASE_PERCENT = 70;

}

GameEngine::GameEngine(const MatchArgs& args)
    : args_(args),
      config_(args.config != nullptr ? *args.config : GameConfig()),
      board_(),
      referee_(board_, args_) {
    if (!config_.initial_board.empty() && !board_.load_layout(config_.initial_board)) {
        std::fprintf(stderr, "[Mesa %d | Partida %d] Disposicion inicial invalida, se usa la por defecto\n",
                     args_.table_id, args_.match_id);
        board_.load_default_layout();
    }
}

unsigned int GameEngine::seed_for(int piece) const {
    unsigned int base = config_.seed != 0 ? config_.seed : static_cast<unsigned int>(std::time(nullptr));
    unsigned int mixed = base * 2654435761u;
    mixed ^= static_cast<unsigned int>(args_.match_id) * 40503u + 1u;
    mixed ^= static_cast<unsigned int>(piece) * 2246822519u + 7u;
    return mixed;
}

int GameEngine::random_delay(unsigned int& seed) const {
    int lo = std::max(0, config_.min_delay_ms);
    int hi = std::max(lo, config_.max_delay_ms);
    return lo + static_cast<int>(rand_r(&seed) % static_cast<unsigned int>(hi - lo + 1));
}

void GameEngine::pause_ms(int ms) {
    while (ms > 0 && !referee_.should_stop()) {
        int slice = std::min(ms, SLEEP_SLICE_MS);
        usleep(static_cast<useconds_t>(slice) * 1000u);
        ms -= slice;
    }
}

void* GameEngine::puma_entry(void* arg) {
    Context* ctx = static_cast<Context*>(arg);
    ctx->engine->puma_loop(ctx->seed);
    return nullptr;
}

void* GameEngine::dog_entry(void* arg) {
    Context* ctx = static_cast<Context*>(arg);
    ctx->engine->dog_loop(ctx->piece, ctx->start, ctx->seed);
    return nullptr;
}

void GameEngine::puma_loop(unsigned int seed) {
    int cycles = 0; 
    while (!referee_.should_stop()) {
        if (config_.abandon_side == 1 && config_.abandon_after > 0 && cycles >= config_.abandon_after) {
            referee_.declare(MatchResult::ABANDON_PUMA);
            break;
        }
        cycles++;
        
        pause_ms(random_delay(seed));
        if (referee_.should_stop()) {
            break;
        }
        int pos = board_.puma_position();
        std::vector<Jump> captures;
        for (const Jump& j : board_.jumps(pos)) {
            if (board_.owner(j.over) >= 1 && board_.owner(j.land) == CELL_EMPTY) {
                captures.push_back(j);
            }
        }
        if (!captures.empty()) {
            const Jump& pick = captures[rand_r(&seed) % captures.size()];
            if (referee_.request_jump(pos, pick.over, pick.land)) {
                continue;
            }
        }
        std::vector<int> options;
        for (int n : board_.neighbors(pos)) {
            if (board_.owner(n) == CELL_EMPTY) {
                options.push_back(n);
            }
        }
        if (!options.empty()) {
            int target = options[rand_r(&seed) % options.size()];
            referee_.request_move(PIECE_PUMA, pos, target);
        }
    }
}

int GameEngine::pick_dog_target(int pos, const std::vector<int>& options, unsigned int& seed) const {
    int puma = board_.puma_position();
    std::vector<int> safe;
    for (int n : options) {
        bool exposed = false;
        for (const Jump& j : board_.jumps(puma)) {
            if (j.over == n && (board_.owner(j.land) == CELL_EMPTY || j.land == pos)) {
                exposed = true;
            }
        }
        if (!exposed) {
            safe.push_back(n);
        }
    }
    const std::vector<int>& pool = safe.empty() ? options : safe;
    if (puma < 0 || rand_r(&seed) % 100 >= DOG_CHASE_PERCENT) {
        return pool[rand_r(&seed) % pool.size()];
    }
    int best = board_.distance(pool[0], puma);
    for (int n : pool) {
        best = std::min(best, board_.distance(n, puma));
    }
    std::vector<int> closest;
    for (int n : pool) {
        if (board_.distance(n, puma) == best) {
            closest.push_back(n);
        }
    }
    return closest[rand_r(&seed) % closest.size()];
}

void GameEngine::dog_loop(int piece, int start, unsigned int seed) {
    int pos = start;
    int cycles = 0; 
    
    while (!referee_.should_stop()) {
        if (config_.abandon_side == 2 && config_.abandon_after > 0 && cycles >= config_.abandon_after) {
            referee_.declare(MatchResult::ABANDON_DOGS);
            break;
        }
        cycles++; 
        
        pause_ms(random_delay(seed));
        if (referee_.should_stop() || board_.owner(pos) != piece) {
            break;
        }
        std::vector<int> options;
        for (int n : board_.neighbors(pos)) {
            if (board_.owner(n) == CELL_EMPTY) {
                options.push_back(n);
            }
        }
        if (options.empty()) {
            continue;
        }
        int target = pick_dog_target(pos, options, seed);
        if (referee_.request_move(piece, pos, target)) {
            pos = target;
        }
    }
}

MatchResult GameEngine::run() {
    if (!referee_.start()) {
        return MatchResult::MATCH_ABORTED_SIGINT;
    }

    int dogs = board_.dog_count();
    std::vector<Context> contexts(static_cast<std::size_t>(dogs) + 1);
    std::vector<pthread_t> threads;
    threads.reserve(static_cast<std::size_t>(dogs) + 1);

    contexts[0] = {this, PIECE_PUMA, board_.puma_position(), seed_for(PIECE_PUMA)};
    pthread_t tid;
    bool ok = true;
    if (pthread_create(&tid, nullptr, &GameEngine::puma_entry, &contexts[0]) == 0) {
        threads.push_back(tid);
    } else {
        referee_.declare(MatchResult::ABANDON_PUMA);
        ok = false;
    }

    for (int d = 1; d <= dogs && ok; ++d) {
        contexts[static_cast<std::size_t>(d)] = {this, d, board_.dog_start(d), seed_for(d)};
        if (pthread_create(&tid, nullptr, &GameEngine::dog_entry, &contexts[static_cast<std::size_t>(d)]) == 0) {
            threads.push_back(tid);
        } else {
            referee_.declare(MatchResult::ABANDON_DOGS);
            ok = false;
        }
    }

    referee_.join();
    for (pthread_t t : threads) {
        pthread_join(t, nullptr);
    }
    return referee_.result();
}

MatchResult run_game_match(MatchArgs& args) {
    GameEngine engine(args);
    return engine.run();
}