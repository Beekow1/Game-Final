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
                bool is_shop = getNode(x, y).room && getNode(x,y).room->kind == RoomKind::Shop;
                std::cout << (is_shop ? "$" : "%");
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
    active_rooms.front()->kind = RoomKind::Start; 
    active_rooms.back()->kind = RoomKind::End;
}

void ProcGen::addRoom(GridNode& node, GridNode& start)
{
    auto newRoom = std::make_shared<Room>();

    newRoom->world_x = static_cast<float>(node.pos_x - start.pos_x) * params.room_scale;
    newRoom->world_y = static_cast<float>(node.pos_y - start.pos_y) * params.room_scale;

    newRoom->grid_x = node.pos_x; 
    newRoom->grid_y = node.pos_y;

    node.room = newRoom;

    active_rooms.push_back(newRoom);
}

std::shared_ptr<Room> ProcGen::addShopRoom()
{
    std::shared_ptr <Room> start_room = nullptr; 
    for (auto const &r : active_rooms) 
    {
        if (r->kind == RoomKind::Start)
        { 
            start_room = r;
            break; 
        }
    }
    if (!start_room) { return nullptr; }
    GridNode &start = getNode(start_room->grid_x, start_room->grid_y);

    // candidates (only rooms that have the above room free for now)
    std::vector<std::shared_ptr<Room>> hosts;
    for (auto const &r : active_rooms)
    {
        if (r->kind != RoomKind::Normal) { continue; }

        int above_y = r->grid_y + 1; 
        if (above_y >= params.rows) { continue; }

        GridNode &above = getNode(r->grid_x, above_y); 
        if (above.occupied) { continue; }

        hosts.push_back(r);
    }

    if (hosts.empty()) { return nullptr; }

    std::shared_ptr<Room> host = hosts[getRand(0, (int)hosts.size() - 1)];

    GridNode &node = getNode(host->grid_x, host->grid_y + 1);
    node.occupied = true; 
    node.pos_x = host->grid_x; 
    node.pos_y = host->grid_y + 1; 
    addRoom(node, start);

    std::shared_ptr<Room> shop = node.room; 
    shop->kind = RoomKind::Shop;

    host->doors |= Doors::Up; 
    shop->doors |= Doors::Down;

    return shop;
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

void ProcGen::addRandomRoom(int door)
{
    glm::vec2 check_dir = glm::vec2(0.0f);

    if (door == Doors::Right || door == Doors::Left)
    {
        (door == Doors::Right) ? check_dir = glm::vec2(1.0f, 0.0f) : check_dir = glm::vec2(-1.0f, 0.0f);
    }
    if (door == Doors::Up || door == Doors::Down)
    {
        (door == Doors::Up) ? check_dir = glm::vec2(0.0f, 1.0f) : check_dir = glm::vec2(0.0f, -1.0f);
    }

    int check_dx = static_cast<int>(check_dir.x);
    int check_dy = static_cast<int>(check_dir.y);

    int start_x = (check_dx < 0) ? -check_dx : 0;
    int end_x   = (check_dx > 0) ? params.cols - check_dx : params.cols;

    int start_y = (check_dy < 0) ? -check_dy : 0;
    int end_y   = (check_dy > 0) ? params.rows - check_dy : params.rows;

    std::vector<GridNode> candidate_rooms;
    std::cout << check_dx << ", " << check_dy << std::endl;

    for (int y = start_y; y < end_y; y++)
    {
        for (int x = start_x; x < end_x; x++)
        {
            if (getNode(x + check_dx, y + check_dy).occupied && !getNode(x, y).occupied)
            {
                candidate_rooms.push_back(getNode(x + check_dx, y + check_dy));
            }
        }
    }

    if (candidate_rooms.empty()) { return; }

    int k = getRand(0, (int)candidate_rooms.size() - 1);

    GridNode& existing = getNode(candidate_rooms[k].pos_x, candidate_rooms[k].pos_y);
    
    int new_x = existing.pos_x - check_dx;
    int new_y = existing.pos_y - check_dy;
    GridNode& new_node = getNode(new_x, new_y);

    GridNode* start = nullptr;
    for (int y = 0; y < params.rows; y++)
    {
        for (int x = 0; x < params.cols; x++)
        {
            if (getNode(x, y).start) { start = &getNode(x, y); }
        }
    }

    new_node.occupied = true;
    new_node.pos_x = new_x;
    new_node.pos_y = new_y;
    addRoom(new_node, *start);

    int opposite = ((door << 2) | (door >> 2)) & 0xF;
    existing.room->doors |= opposite;
    new_node.room->doors |= door; 
}
