#include "ProcGen.hpp"

void ProcGen::clearRoom(int x, int y)
{
    GridNode& node = getNode(x, y);
    node.occupied = false;
    node.start = false;
    node.locked = false;
    node.room = nullptr;
}

void ProcGen::printGrid()
{
    for (int y = params.rows - 1; y >= 0; y--)
    {
        for (int x = 0; x < params.cols; x++)
        {
            std::cout << "|";
            
            if (getNode(x,y).start)
            {
                std::cout << "*";
            }
            else if (getNode(x,y).occupied)
            {
                
                std::cout << "%";
            }
            else
            {
                std::cout << " "; 
            }
        }  
        std::cout << "|";
        std::cout << std::endl;
    }
}

void ProcGen::generateLevel()
{
    active_rooms.clear();
    active_rooms.reserve(params.path_len);

    int x = getRand(params.edge_buf, params.cols - 1 - params.edge_buf);
    int y = getRand(params.edge_buf, params.rows - 1 - params.edge_buf);

    GridNode& start = getNode(x, y);
    start.occupied = true;
    start.start = true;
    start.pos_x = x;
    start.pos_y = y;
    addRoom(start, start);
    node_stack.push(&start);
    int last = -1;

    while (node_stack.size() < params.path_len)
    {
        if (node_stack.size() == 0) { break; }

        auto valid_moves = validNeighbours(x, y);
        if (valid_moves.empty())
        {
            if (node_stack.size() == 1) { break; }    

            int in = getNode(x,y).room->doors; 
            clearRoom(x,y);
            getNode(x,y).locked = true;
            active_rooms.pop_back();
            node_stack.pop();
            x = node_stack.top()->pos_x; 
            y = node_stack.top()->pos_y;
            getNode(x, y).room->doors &= ~(((in << 2) | (in >> 2)) & 0xF);
            last = -1;
            continue;
        }
        
        std::vector<float> weights;
        for (int m : valid_moves)
        {
            float w = (m == last) ? params.straightness_bias : 1.0f - params.straightness_bias;
            weights.push_back(std::max(w, 1e-4f));
        }
        std::discrete_distribution<int> pick(weights.begin(), weights.end());
        int k = valid_moves[pick(gen)];

        getNode(x, y).room->doors |= 1 << k; 
        x += dx[k];
        y += dy[k];
        getNode(x, y).occupied = true;
        getNode(x, y).pos_x = x;
        getNode(x, y).pos_y = y;

        node_stack.push(&getNode(x, y));
        last = k;

        addRoom(getNode(x, y), start);
        getNode(x, y).room->doors |= 1 << ((k + 2) % 4);
    }

    active_rooms.reserve(node_stack.size());
    
}

void ProcGen::addRoom(GridNode& node, GridNode& start)
{
    auto newRoom = std::make_shared<Room>();

    newRoom->world_x = static_cast<float>(node.pos_x - start.pos_x) * params.room_scale;
    newRoom->world_y = static_cast<float>(node.pos_y - start.pos_y) * params.room_scale;

    node.room = newRoom;

    active_rooms.push_back(newRoom);
}

std::vector<int> ProcGen::validNeighbours(int x, int y)
{
    std::vector<int> valid_dir;
    for (int i = 0; i < 4; i++)
    {
        int next_x = x + dx[i];
        int next_y = y + dy[i];

        if (next_x >= 0 && next_x < params.cols && next_y >= 0 && next_y < params.rows)
        {
            if (!getNode(next_x, next_y).occupied && !getNode(next_x, next_y).locked)
            {
                valid_dir.push_back(i);
            }
        }
    }

    return valid_dir;
}