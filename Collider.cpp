#include "Collider.hpp"

bool checkCollision(const AABB& a, const AABB& b)
{
    return (
		a.min.x <= b.max.x &&
		a.max.x >= b.min.x &&
		a.min.y <= b.max.y &&
		a.max.y >= b.min.y
	);
}

glm::vec3 getAABBFixVec(const AABB& a, const AABB& b) 
{
    float overlap_x1 = a.max.x - b.min.x;
    float overlap_x2 = b.max.x - a.min.x;
    float overlap_y1 = a.max.y - b.min.y;
    float overlap_y2 = b.max.y - a.min.y;

    if (overlap_x1 <= 0.0f || overlap_x2 <= 0.0f ||
        overlap_y1 <= 0.0f || overlap_y2 <= 0.0f) 
    {
        return glm::vec3(0.0f);
    }

    float push_x = (overlap_x1 < overlap_x2) ? -overlap_x1 : overlap_x2;
    float push_y = (overlap_y1 < overlap_y2) ? -overlap_y1 : overlap_y2;

    float abs_x = std::abs(push_x);
    float abs_y = std::abs(push_y);

    if (abs_x <= abs_y) 
    {
        return glm::vec3(push_x, 0.0f, 0.0f);
    } 
    else 
    {
        return glm::vec3(0.0f, push_y, 0.0f);
    }
}