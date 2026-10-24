#include "Server.hpp"

#include <iostream>
#include <string>
#include <cstring>

static std::string matchResultStr(MatchResult r) {
    switch (r) {
        case MatchResult::WINNER_PUMA:          return "GANA PUMA";
        case MatchResult::WINNER_DOGS:          return "GANAN PERROS";
        case MatchResult::ABANDON_PUMA:         return "ABANDONO PUMA";
        case MatchResult::ABANDON_DOGS:         return "ABANDONO PERROS";
        case MatchResult::MATCH_ABORTED_SIGINT: return "ABORTADA (SIGINT)";
    }
    return "DESCONOCIDO";
}

Server::Server(int M, const GameConfig& config, std::atomic<bool>& global_abort)
    : M_(M),
      config_(config),
      global_abort_(global_abort)
{
    sem_init(&available_tables_, 0, static_cast<unsigned int>(M_));
    pthread_mutex_init(&threads_mutex_, nullptr);
}

Server::~Server() {
    sem_destroy(&available_tables_);
    pthread_mutex_destroy(&threads_mutex_);
}

void Server::run(Matchmaker& matchmaker) {
    int table_id_counter = 0;

    while (true) {
        PlayerPair pair;

        if (!matchmaker.nextPair(pair)) {
            break;
        }

        while (sem_wait(&available_tables_) == -1) {
            if (global_abort_.load()) {
                matchmaker.close();
                return;
            }
        }

        if (global_abort_.load()) {
            sem_post(&available_tables_);
            matchmaker.close();
            break;
        }

        int match_id = ++next_match_id_;
        int table_id = table_id_counter++ % M_;

        TableArgs* args = new TableArgs{};
        args->server = this;
        args->match_args.match_id      = match_id;
        args->match_args.table_id      = table_id;
        args->match_args.player_puma_id = pair.puma_id;
        args->match_args.player_dogs_id = pair.dogs_id;
        args->match_args.config        = &config_;
        args->match_args.global_abort  = &global_abort_;

        std::cout << "[Servidor] Partida #" << match_id
                  << " iniciada | Mesa " << table_id
                  << " | Puma=J" << pair.puma_id
                  << " Perros=J" << pair.dogs_id << "\n"
                  << std::flush;

        pthread_t tid;
        if (pthread_create(&tid, nullptr, tableThreadFunc, args) != 0) {
            std::perror("pthread_create");
            delete args;
            sem_post(&available_tables_);
            continue;
        }

        pthread_mutex_lock(&threads_mutex_);
        active_threads_.push_back(tid);
        pthread_mutex_unlock(&threads_mutex_);
    }
}

void Server::waitAll() {
    pthread_mutex_lock(&threads_mutex_);
    std::vector<pthread_t> threads_copy = active_threads_;
    pthread_mutex_unlock(&threads_mutex_);

    for (pthread_t tid : threads_copy) {
        pthread_join(tid, nullptr);
    }
}

void* Server::tableThreadFunc(void* arg) {
    TableArgs* targs = static_cast<TableArgs*>(arg);
    Server* server   = targs->server;

    int match_id    = targs->match_args.match_id;
    int player_puma = targs->match_args.player_puma_id;
    int player_dogs = targs->match_args.player_dogs_id;

    MatchResult result = run_game_match(targs->match_args);

    std::cout << "[Servidor] Partida #" << match_id
              << " finalizada | Puma=J" << player_puma
              << " Perros=J" << player_dogs
              << " → " << matchResultStr(result) << "\n"
              << std::flush;

    sem_post(&server->available_tables_);

    delete targs;
    return nullptr;
}
