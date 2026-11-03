#pragma once
#include <vector>
#include <iostream>
#include <random>
#include <array>
#include <stack>
#include <glm/glm.hpp>

//enum class Direction : int { Right = 0, Down = 1, Left = 2, Up = 3, None = -1 };

namespace Doors
{
    constexpr int None  = 0;
    constexpr int Right = 1 << 0;
    constexpr int Down  = 1 << 1;
    constexpr int Left  = 1 << 2;
    constexpr int Up    = 1 << 3;
}

struct Room
{
    size_t width = 1, height = 1;
    int doors = Doors::None;
    float world_x = 0.0f, world_y = 0.0f;
};

struct GridNode
{
    bool occupied = false;
    bool start = false;
    bool locked = false;
    int pos_x;
    int pos_y;
    std::shared_ptr<Room> room = nullptr;
};

struct GenParams
{
    int rows = 10;
    int cols = 10;
    size_t path_len = 10;
    int edge_buf = 2;
    unsigned int seed = 42;
    float straightness_bias = 0.65f;

    float room_scale = 512.0f;
};

struct ProcGen
{
    GenParams params;

    std::vector<GridNode> grid;
    std::stack<GridNode*> node_stack;

    std::vector<std::shared_ptr<Room>> active_rooms;

    const std::array<int , 4> dx = {1, 0, -1, 0};
    const std::array<int , 4> dy = {0, -1, 0, 1};

    std::mt19937 gen;

    ProcGen(const GenParams& p = GenParams{}) 
        : params(p), 
          grid(p.rows * p.cols), 
          gen(p.seed) 
    {}

    template <typename T>
    T getRand(T min, T max)
    {
        std::uniform_int_distribution<T> distrib(min, max);
        return distrib(gen);
    }

    void generateLevel();

    std::vector<int> validNeighbours(int x, int y);

    GridNode& getNode(int x, int y) { return grid[x + y * params.cols]; }
    void clearRoom(int x, int y);

    void printGrid();

    float toCartesian(float x);
    void addRoom(GridNode& node, GridNode& start);
};
