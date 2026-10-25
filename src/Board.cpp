#include "Board.hpp"

#include <algorithm>
#include <cstdio>

Board::Board() : puma_pos_(-1), dog_count_(0) {
    for (int i = 0; i < KOMIKAN_NODES; ++i) {
        pthread_mutex_init(&locks_[i], nullptr);
        owner_[i].store(CELL_EMPTY);
    }
    for (int i = 0; i <= KOMIKAN_DOGS; ++i) {
        dog_start_[i] = -1;
    }
    build_graph();
    load_default_layout();
}

Board::~Board() {
    for (int i = 0; i < KOMIKAN_NODES; ++i) {
        pthread_mutex_destroy(&locks_[i]);
    }
}

void Board::add_edge(int a, int b) {
    if (std::find(adj_[a].begin(), adj_[a].end(), b) == adj_[a].end()) {
        adj_[a].push_back(b);
    }
    if (std::find(adj_[b].begin(), adj_[b].end(), a) == adj_[b].end()) {
        adj_[b].push_back(a);
    }
}

void Board::build_graph() {
    std::vector<std::vector<int>> lines;
    for (int r = 0; r < KOMIKAN_GRID_SIDE; ++r) {
        std::vector<int> row;
        for (int c = 0; c < KOMIKAN_GRID_SIDE; ++c) {
            row.push_back(r * KOMIKAN_GRID_SIDE + c);
        }
        lines.push_back(row);
    }
    lines.push_back({0, 5, 10, 15, 20});
    lines.push_back({NODE_THRONE_LEFT, 1, 6, 11, 16, 21});
    lines.push_back({NODE_THRONE_TOP, 2, 7, 12, 17, 22});
    lines.push_back({NODE_THRONE_RIGHT, 3, 8, 13, 18, 23});
    lines.push_back({4, 9, 14, 19, 24});
    lines.push_back({NODE_THRONE_TOP, NODE_THRONE_LEFT, 0});
    lines.push_back({NODE_THRONE_TOP, NODE_THRONE_RIGHT, 4});
    lines.push_back({NODE_THRONE_LEFT, NODE_THRONE_RIGHT});

    for (const std::vector<int>& line : lines) {
        for (std::size_t i = 0; i + 1 < line.size(); ++i) {
            add_edge(line[i], line[i + 1]);
        }
        for (std::size_t i = 0; i + 2 < line.size(); ++i) {
            jumps_[line[i]].push_back({line[i + 1], line[i + 2]});
            jumps_[line[i + 2]].push_back({line[i + 1], line[i]});
        }
    }

    for (int src = 0; src < KOMIKAN_NODES; ++src) {
        for (int i = 0; i < KOMIKAN_NODES; ++i) {
            dist_[src][i] = -1;
        }
        std::vector<int> queue{src};
        dist_[src][src] = 0;
        for (std::size_t head = 0; head < queue.size(); ++head) {
            int cur = queue[head];
            for (int n : adj_[cur]) {
                if (dist_[src][n] < 0) {
                    dist_[src][n] = dist_[src][cur] + 1;
                    queue.push_back(n);
                }
            }
        }
    }
}

void Board::clear_cells() {
    for (int i = 0; i < KOMIKAN_NODES; ++i) {
        owner_[i].store(CELL_EMPTY);
    }
    for (int i = 0; i <= KOMIKAN_DOGS; ++i) {
        dog_start_[i] = -1;
    }
    dog_count_ = 0;
    puma_pos_.store(-1);
}

void Board::load_default_layout() {
    clear_cells();
    set_owner(NODE_THRONE_TOP, PIECE_PUMA);
    for (int d = 1; d <= KOMIKAN_DOGS; ++d) {
        set_owner(KOMIKAN_DOGS + d, d);
        dog_start_[d] = KOMIKAN_DOGS + d;
    }
    dog_count_ = KOMIKAN_DOGS;
}

bool Board::load_layout(const std::string& layout) {
    std::vector<int> cells;
    for (char ch : layout) {
        switch (ch) {
            case 'P':
            case 'p':
                cells.push_back(PIECE_PUMA);
                break;
            case 'D':
            case 'd':
                cells.push_back(1);
                break;
            case '.':
            case '-':
            case '_':
            case '0':
                cells.push_back(CELL_EMPTY);
                break;
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case ',':
            case '|':
                break;
            default:
                return false;
        }
    }
    if (static_cast<int>(cells.size()) != KOMIKAN_NODES) {
        return false;
    }
    int pumas = 0;
    int dogs = 0;
    for (int v : cells) {
        if (v == PIECE_PUMA) {
            ++pumas;
        } else if (v == 1) {
            ++dogs;
        }
    }
    if (pumas != 1 || dogs < 1 || dogs > KOMIKAN_DOGS) {
        return false;
    }
    clear_cells();
    int next_dog = 1;
    for (int i = 0; i < KOMIKAN_NODES; ++i) {
        int node = i == 0 ? NODE_THRONE_TOP
                          : (i == 1 ? NODE_THRONE_LEFT : (i == 2 ? NODE_THRONE_RIGHT : i - 3));
        if (cells[i] == PIECE_PUMA) {
            set_owner(node, PIECE_PUMA);
        } else if (cells[i] == 1) {
            set_owner(node, next_dog);
            dog_start_[next_dog] = node;
            ++next_dog;
        }
    }
    dog_count_ = dogs;
    return true;
}

int Board::owner(int node) const {
    return owner_[node].load();
}

void Board::set_owner(int node, int piece) {
    owner_[node].store(piece);
    if (piece == PIECE_PUMA) {
        puma_pos_.store(node);
    }
}

int Board::puma_position() const {
    return puma_pos_.load();
}

int Board::dog_count() const {
    return dog_count_;
}

int Board::dog_start(int piece) const {
    return dog_start_[piece];
}

const std::vector<int>& Board::neighbors(int node) const {
    return adj_[node];
}

const std::vector<Jump>& Board::jumps(int node) const {
    return jumps_[node];
}

bool Board::adjacent(int a, int b) const {
    const std::vector<int>& list = adj_[a];
    return std::find(list.begin(), list.end(), b) != list.end();
}

bool Board::valid_jump(int from, int over, int land) const {
    for (const Jump& j : jumps_[from]) {
        if (j.over == over && j.land == land) {
            return true;
        }
    }
    return false;
}

int Board::distance(int a, int b) const {
    return dist_[a][b];
}

void Board::lock_nodes(std::vector<int>& nodes) {
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    for (int n : nodes) {
        pthread_mutex_lock(&locks_[n]);
    }
}

void Board::unlock_nodes(const std::vector<int>& nodes) {
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
        pthread_mutex_unlock(&locks_[*it]);
    }
}

std::string Board::render() {
    std::vector<int> nodes(KOMIKAN_NODES);
    for (int i = 0; i < KOMIKAN_NODES; ++i) {
        nodes[i] = i;
    }
    lock_nodes(nodes);
    char sym[KOMIKAN_NODES];
    for (int i = 0; i < KOMIKAN_NODES; ++i) {
        int o = owner(i);
        sym[i] = o == CELL_EMPTY ? '.' : (o == PIECE_PUMA ? 'P' : 'D');
    }
    unlock_nodes(nodes);

    std::string out;
    char line[96];
    std::snprintf(line, sizeof line, "          [%c]\n", sym[NODE_THRONE_TOP]);
    out += line;
    std::snprintf(line, sizeof line, "      [%c]-----[%c]\n", sym[NODE_THRONE_LEFT], sym[NODE_THRONE_RIGHT]);
    out += line;
    for (int r = 0; r < KOMIKAN_GRID_SIDE; ++r) {
        int b = r * KOMIKAN_GRID_SIDE;
        std::snprintf(line, sizeof line, "  [%c]-[%c]-[%c]-[%c]-[%c]\n",
                      sym[b], sym[b + 1], sym[b + 2], sym[b + 3], sym[b + 4]);
        out += line;
    }
    return out;
}