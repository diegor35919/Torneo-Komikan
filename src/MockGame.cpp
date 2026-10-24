#include "common_types.hpp"

#include <cstdlib>
#include <unistd.h>
#include <iostream>

MatchResult run_game_match(MatchArgs& args) {
    unsigned int local_seed = args.config ? args.config->seed : 0u;
    local_seed ^= static_cast<unsigned int>(args.match_id) * 2654435761u;
    if (local_seed == 0) local_seed = 1;

    int duration_ms = 200 + static_cast<int>(rand_r(&local_seed) % 801);
    int elapsed_ms  = 0;
    const int step_ms = 10;

    std::cout << "[MockGame] Partida #" << args.match_id
              << " (Mesa " << args.table_id << ")"
              << " J" << args.player_puma_id << " vs J" << args.player_dogs_id
              << " — duración simulada: " << duration_ms << "ms\n"
              << std::flush;

    while (elapsed_ms < duration_ms) {
        if (args.global_abort && args.global_abort->load()) {
            std::cout << "[MockGame] Partida #" << args.match_id
                      << " abortada por SIGINT\n" << std::flush;
            return MatchResult::MATCH_ABORTED_SIGINT;
        }

        usleep(static_cast<useconds_t>(step_ms) * 1000u);
        elapsed_ms += step_ms;
    }

    MatchResult result = (rand_r(&local_seed) % 2 == 0)
        ? MatchResult::WINNER_PUMA
        : MatchResult::WINNER_DOGS;

    return result;
}
