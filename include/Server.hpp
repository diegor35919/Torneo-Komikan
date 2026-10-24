#ifndef SERVER_HPP
#define SERVER_HPP

#include "common_types.hpp"
#include "Matchmaker.hpp"
#include <semaphore.h>
#include <vector>
#include <pthread.h>
#include <atomic>

class Server {
public:
    Server(int M, const GameConfig& config, std::atomic<bool>& global_abort);
    ~Server();

    
    void run(Matchmaker& matchmaker);

    
    void waitAll();

private:
    struct TableArgs {
        Server* server;
        MatchArgs match_args;
    };

    int M_;
    const GameConfig& config_;
    std::atomic<bool>& global_abort_;

    sem_t available_tables_;          

    pthread_mutex_t threads_mutex_;
    std::vector<pthread_t> active_threads_;

    int next_match_id_{0};

    static void* tableThreadFunc(void* arg);
};

#endif 
