#ifndef COMMON_TYPES_HPP
#define COMMON_TYPES_HPP

#include <string>
#include <atomic>

constexpr int BOARD_NODES = 30;
constexpr int TOTAL_DOGS = 12;
constexpr int PUMA_WIN_CAPTURES = 6;

struct GameConfig {
    int min_delay_ms{50};
    int max_delay_ms{300};
    unsigned int seed{0};
    std::string initial_board{};
};

enum class MatchResult {
    WINNER_PUMA,
    WINNER_DOGS,
    ABANDON_PUMA,
    ABANDON_DOGS,
    MATCH_ABORTED_SIGINT
};

struct MatchArgs {
    int match_id{0};
    int table_id{0};
    int player_puma_id{0};
    int player_dogs_id{0};
    const GameConfig* config{nullptr};
    const std::atomic<bool>* global_abort{nullptr};
};

MatchResult run_game_match(MatchArgs& args);

#endif
