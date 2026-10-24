#ifndef MATCHMAKER_HPP
#define MATCHMAKER_HPP

#include <queue>
#include <utility>
#include <pthread.h>

struct PlayerPair {
    int puma_id;
    int dogs_id;
};

class Matchmaker {
public:
    explicit Matchmaker(int total_players);
    ~Matchmaker();

    void addPlayer(int player_id);
    bool nextPair(PlayerPair& out);
    void close();
    int totalPairs() const;

private:
    int total_players_;
    int pairs_expected_;
    int pairs_delivered_{0};
    bool closed_{false};

    std::queue<int> waiting_;
    pthread_mutex_t mutex_;
    pthread_cond_t  cond_pair_ready_;
    pthread_cond_t  cond_space_;
};

#endif
