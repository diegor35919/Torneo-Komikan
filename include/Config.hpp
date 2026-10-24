#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "common_types.hpp"
#include <string>

class Config {
public:
    int N{0};
    int M{0};
    GameConfig game;

    static Config parse(int argc, char* argv[]);

private:
    static void loadFile(const std::string& path, GameConfig& cfg);
};

#endif
