#include "Config.hpp"
#include "Matchmaker.hpp"
#include "Server.hpp"
#include "Signals.hpp"

#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <atomic>

struct TestProducerArgs {
    Matchmaker* mm;
    int N;
};

static void* testProducer(void* arg) {
    auto* a = static_cast<TestProducerArgs*>(arg);
    for (int i = 1; i <= a->N; ++i) {
        usleep(1000);
        a->mm->addPlayer(i);
    }
    a->mm->close();
    return nullptr;
}

int main() {
    std::cout << "═══════════════════════════════════════\n";
    std::cout << "   TEST DE ESTRÉS — Servidor + Matchmaker\n";
    std::cout << "═══════════════════════════════════════\n";

    Signals::setup();

    const int N = 50;
    const int M = 4;

    GameConfig game_cfg;
    game_cfg.min_delay_ms = 50;
    game_cfg.max_delay_ms = 300;
    game_cfg.seed         = 42;

    std::cout << "  N=" << N << "  M=" << M << "  seed=" << game_cfg.seed << "\n\n";

    Matchmaker matchmaker(N);
    Server server(M, game_cfg, Signals::g_stop_requested);

    TestProducerArgs pargs{&matchmaker, N};
    pthread_t prod;
    pthread_create(&prod, nullptr, testProducer, &pargs);

    server.run(matchmaker);
    server.waitAll();
    pthread_join(prod, nullptr);

    int expected_pairs = N / 2;
    int total_pairs    = matchmaker.totalPairs();

    std::cout << "\n═══════════════════════════════════════\n";
    if (total_pairs == expected_pairs) {
        std::cout << "  TEST PASSED ✓ — " << expected_pairs << " pares procesados correctamente.\n";
    } else {
        std::cout << "  TEST FAILED ✗ — Se esperaban " << expected_pairs
                  << " pares, totalPairs()=" << total_pairs << "\n";
        return 1;
    }
    std::cout << "═══════════════════════════════════════\n";

    return 0;
}
