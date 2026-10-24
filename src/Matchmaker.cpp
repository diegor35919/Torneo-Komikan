#include "Matchmaker.hpp"

#include <cassert>
#include <iostream>

Matchmaker::Matchmaker(int total_players)
    : total_players_(total_players),
      pairs_expected_(total_players / 2)
{
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_pair_ready_, nullptr);
    pthread_cond_init(&cond_space_, nullptr);
}

Matchmaker::~Matchmaker() {
    pthread_cond_destroy(&cond_space_);
    pthread_cond_destroy(&cond_pair_ready_);
    pthread_mutex_destroy(&mutex_);
}

void Matchmaker::addPlayer(int player_id) {
    pthread_mutex_lock(&mutex_);

    waiting_.push(player_id);


    if (static_cast<int>(waiting_.size()) >= 2) {
        pthread_cond_signal(&cond_pair_ready_);
    }

    pthread_mutex_unlock(&mutex_);
}

bool Matchmaker::nextPair(PlayerPair& out) {
    pthread_mutex_lock(&mutex_);


    while (static_cast<int>(waiting_.size()) < 2 && !closed_) {
        pthread_cond_wait(&cond_pair_ready_, &mutex_);
    }


    if (static_cast<int>(waiting_.size()) >= 2) {
        int first  = waiting_.front(); waiting_.pop();
        int second = waiting_.front(); waiting_.pop();

        out.puma_id = first;
        out.dogs_id = second;

        ++pairs_delivered_;

        pthread_mutex_unlock(&mutex_);
        return true;
    }


    pthread_mutex_unlock(&mutex_);
    return false;
}

void Matchmaker::close() {
    pthread_mutex_lock(&mutex_);
    closed_ = true;
    
    pthread_cond_broadcast(&cond_pair_ready_);
    pthread_mutex_unlock(&mutex_);
}

int Matchmaker::totalPairs() const {
    return pairs_expected_;
}
