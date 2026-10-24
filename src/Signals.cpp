#include "Signals.hpp"

#include <signal.h>
#include <cstring>
#include <iostream>

namespace Signals {

std::atomic<bool> g_stop_requested{false};

static void sigint_handler(int) {
    g_stop_requested.store(true);
}

void setup() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        std::perror("sigaction");
    }
}

bool stopRequested() {
    return g_stop_requested.load();
}

}
