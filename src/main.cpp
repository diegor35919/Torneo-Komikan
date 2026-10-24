#include "Signals.hpp"
#include "Config.hpp"
#include "Matchmaker.hpp"
#include "Server.hpp"

#include <iostream>
#include <unistd.h>
#include <pthread.h>


struct ProducerArgs {
    Matchmaker* matchmaker;
    int N;
};


static void* producerThreadFunc(void* arg) {
    auto* pargs = static_cast<ProducerArgs*>(arg);
    Matchmaker* mm = pargs->matchmaker;
    int N = pargs->N;

    for (int i = 1; i <= N; ++i) {
        
        usleep(5000); 

        if (Signals::stopRequested()) {
            std::cout << "[Productor] SIGINT detectado, dejando de agregar jugadores.\n"
                      << std::flush;
            break;
        }

        std::cout << "[Productor] Jugador J" << i << " se unió al torneo.\n"
                  << std::flush;
        mm->addPlayer(i);
    }


    mm->close();
    std::cout << "[Productor] Todos los jugadores han sido registrados.\n"
              << std::flush;

    return nullptr;
}


int main(int argc, char* argv[]) {

    Signals::setup();


    Config cfg;
    try {
        cfg = Config::parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    std::cout << "TORNEO DE KOMIKAN CONCURRENTE\n";
    std::cout << "  Jugadores (N) : " << cfg.N << "\n";
    std::cout << "  Mesas máx (M) : " << cfg.M << "\n";
    std::cout << "  Delay mínimo  : " << cfg.game.min_delay_ms << " ms\n";
    std::cout << "  Delay máximo  : " << cfg.game.max_delay_ms << " ms\n";
    std::cout << "  Semilla RNG   : " << cfg.game.seed << "\n";
    if (!cfg.game.initial_board.empty()) {
        std::cout << "  Tablero init  : " << cfg.game.initial_board << "\n";
    }
    std::cout << "  Partidas esperadas: " << (cfg.N / 2) << "\n";
    std::cout << "──────────────────────────────────────────────\n"
              << std::flush;


    Matchmaker matchmaker(cfg.N);


    Server server(cfg.M, cfg.game, Signals::g_stop_requested);

    ProducerArgs pargs{&matchmaker, cfg.N};
    pthread_t producer_thread;
    if (pthread_create(&producer_thread, nullptr, producerThreadFunc, &pargs) != 0) {
        std::perror("pthread_create (productor)");
        return 1;
    }

    server.run(matchmaker);


    server.waitAll();

   
    pthread_join(producer_thread, nullptr);


    std::cout << "──────────────────────────────────────────────\n";
    std::cout << "╔══════════════════════════════════════════════╗\n";

    if (Signals::stopRequested()) {
        std::cout << "║   TORNEO INTERRUMPIDO (Ctrl+C)               ║\n";
    } else {
        std::cout << "║   TORNEO FINALIZADO                          ║\n";
    }

    std::cout << "╚══════════════════════════════════════════════╝\n";
    std::cout << "  Partidas completadas: hasta " << (cfg.N / 2) << " pares posibles.\n"
              << std::flush;

    return 0;
}
