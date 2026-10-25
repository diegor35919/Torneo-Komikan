#ifndef KOMIKAN_BOARD_HPP
#define KOMIKAN_BOARD_HPP

#include <atomic>
#include <string>
#include <vector>
#include <pthread.h>

constexpr int KOMIKAN_NODES = 28;
constexpr int KOMIKAN_GRID_SIDE = 5;
constexpr int KOMIKAN_DOGS = 12;
constexpr int KOMIKAN_PUMA_WIN = 6;
constexpr int NODE_THRONE_LEFT = 25;
constexpr int NODE_THRONE_RIGHT = 26;
constexpr int NODE_THRONE_TOP = 27;
constexpr int CELL_EMPTY = -1;
constexpr int PIECE_PUMA = 0;

struct Jump {
    int over;
    int land;
};

class Board {
public:
    Board();
    ~Board();
    Board(const Board&) = delete;
    Board& operator=(const Board&) = delete;

    void load_default_layout();
    bool load_layout(const std::string& layout);

    int owner(int node) const;
    void set_owner(int node, int piece);
    int puma_position() const;
    int dog_count() const;
    int dog_start(int piece) const;

    const std::vector<int>& neighbors(int node) const;
    const std::vector<Jump>& jumps(int node) const;
    bool adjacent(int a, int b) const;
    bool valid_jump(int from, int over, int land) const;
    int distance(int a, int b) const;

    void lock_nodes(std::vector<int>& nodes);
    void unlock_nodes(const std::vector<int>& nodes);

    std::string render();

private:
    void build_graph();
    void add_edge(int a, int b);
    void clear_cells();

    std::vector<int> adj_[KOMIKAN_NODES];
    std::vector<Jump> jumps_[KOMIKAN_NODES];
    pthread_mutex_t locks_[KOMIKAN_NODES];
    std::atomic<int> owner_[KOMIKAN_NODES];
    std::atomic<int> puma_pos_;
    int dist_[KOMIKAN_NODES][KOMIKAN_NODES];
    int dog_start_[KOMIKAN_DOGS + 1];
    int dog_count_;
};

#endif