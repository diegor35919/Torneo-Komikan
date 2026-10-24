#include "Config.hpp"

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

Config Config::parse(int argc, char* argv[]) {
    if (argc < 3) {
        throw std::invalid_argument(
            "Uso: ./torneo N M [config.txt]\n"
            "  N >= 2  : número total de jugadores\n"
            "  M >= 1  : número máximo de mesas simultáneas\n"
        );
    }

    Config cfg;

    try {
        cfg.N = std::stoi(argv[1]);
    } catch (...) {
        throw std::invalid_argument("N debe ser un entero válido.");
    }

    try {
        cfg.M = std::stoi(argv[2]);
    } catch (...) {
        throw std::invalid_argument("M debe ser un entero válido.");
    }

    if (cfg.N < 2) {
        throw std::invalid_argument("N debe ser >= 2 (se necesitan al menos 2 jugadores).");
    }
    if (cfg.M < 1) {
        throw std::invalid_argument("M debe ser >= 1 (se necesita al menos 1 mesa).");
    }

    if (argc >= 4) {
        loadFile(argv[3], cfg.game);
    }

    return cfg;
}

void Config::loadFile(const std::string& path, GameConfig& cfg) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo de configuración: " + path);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto start = line.find_first_not_of(" \t\r\n");
        auto end   = line.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start, end - start + 1);

        if (line.empty() || line[0] == '#') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        auto trim = [](const std::string& s) {
            auto a = s.find_first_not_of(" \t");
            auto b = s.find_last_not_of(" \t");
            if (a == std::string::npos) return std::string{};
            return s.substr(a, b - a + 1);
        };
        key   = trim(key);
        value = trim(value);

        if (key == "min_delay_ms") {
            cfg.min_delay_ms = std::stoi(value);
        } else if (key == "max_delay_ms") {
            cfg.max_delay_ms = std::stoi(value);
        } else if (key == "seed") {
            cfg.seed = static_cast<unsigned int>(std::stoul(value));
        } else if (key == "initial_board") {
            cfg.initial_board = value;
        } else {
            std::cerr << "[Config] Clave desconocida ignorada: '" << key << "'\n";
        }
    }
}
