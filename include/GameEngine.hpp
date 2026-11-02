#ifndef KOMIKAN_GAME_ENGINE_HPP
#define KOMIKAN_GAME_ENGINE_HPP

#include <vector>

#include "Board.hpp"
#include "Referee.hpp"
#include "common_types.hpp"

class GameEngine {
public:
    explicit GameEngine(const MatchArgs& args);
    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;

    MatchResult run();

private:
    struct Context {
        GameEngine* engine;
        int piece;
        int start;
        unsigned int seed;
    };

    static void* puma_entry(void* arg);
    static void* dog_entry(void* arg);

    void puma_loop(unsigned int seed);
    void dog_loop(int piece, int start, unsigned int seed);
    int pick_dog_target(int pos, const std::vector<int>& options, unsigned int& seed) const;
    void pause_ms(int ms);
    int random_delay(unsigned int& seed) const;
    unsigned int seed_for(int piece) const;

    MatchArgs args_;
    GameConfig config_;
    Board board_;
    Referee referee_;
};

#endif