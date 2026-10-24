#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include <atomic>

namespace Signals {
    extern std::atomic<bool> g_stop_requested;

   
    void setup();

   
    bool stopRequested();
}

#endif 
