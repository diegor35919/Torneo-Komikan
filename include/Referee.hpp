#ifndef KOMIKAN_REFEREE_HPP
#define KOMIKAN_REFEREE_HPP

#include <atomic>
#include <vector>
#include <pthread.h>

#include "Board.hpp"
#include "common_types.hpp"

class Referee {
public:
    Referee(Board& board, const MatchArgs& args);
    ~Referee();
    Referee(const Referee&) = delete;
    Referee& operator=(const Referee&) = delete;

    bool start();
    void join();

    bool request_move(int piece, int from, int to);
    bool request_jump(int from, int over, int land);

    void declare(MatchResult result);
    bool should_stop() const;
    MatchResult result();

private:
    struct Event {
        int kind;
        int piece;
        int from;
        int to;
        int over;
        int victim;
        int total;
    };

    static void* judge_entry(void* arg);
    void judge_loop();
    bool aborted() const;
    void decide_locked(MatchResult result);
    void push_event(const Event& e);
    bool puma_trapped();
    void log(const std::string& text);
    std::string describe(const Event& e) const;
    const char* describe(MatchResult r) const;

    Board& board_;
    int match_id_;
    int table_id_;
    int puma_player_;
    int dogs_player_;
    const std::atomic<bool>* abort_flag_;

    pthread_mutex_t mx_;
    pthread_cond_t cv_;
    std::vector<Event> queue_;
    std::atomic<bool> stop_;
    bool decided_;
    MatchResult result_;
    std::atomic<int> captures_;
    int moves_;
    pthread_t judge_;
    bool judge_started_;
};

#endif