#pragma once
#include <glm/glm.hpp>
#include "Scene.hpp"
#include <vector>
#include <iostream>

#include <cstdint>

struct AABB
{
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
};

enum class CollisionLayer : std::uint32_t
{
    Default = 0,
    Level = 1u << 0,
    Player = 1u << 1,
    Enemy = 1u << 2,
};

struct Collider
{
    AABB local_aabb;
    Scene::Transform *transform = nullptr;
    CollisionLayer layer = CollisionLayer::Default;
    std::uint32_t collision_mask = 0;
    
    AABB getWorldSpaceAABB() const
    {
        AABB aabb;
        glm::vec3 world_bl = transform->make_world_from_local() * glm::vec4(local_aabb.min.x, local_aabb.min.y, local_aabb.min.z, 1.0f);
        glm::vec3 world_tl = transform->make_world_from_local() * glm::vec4(local_aabb.min.x, local_aabb.max.y, local_aabb.min.z, 1.0f);
        glm::vec3 world_tr = transform->make_world_from_local() * glm::vec4(local_aabb.max.x, local_aabb.max.y, local_aabb.min.z, 1.0f);
        glm::vec3 world_br = transform->make_world_from_local() * glm::vec4(local_aabb.max.x, local_aabb.min.y, local_aabb.min.z, 1.0f);

        aabb.min = glm::min(glm::min(world_bl, world_tl), glm::min(world_tr, world_br));
        aabb.max = glm::max(glm::max(world_bl, world_tl), glm::max(world_tr, world_br));

        return aabb;
    }

    bool operator==(const Collider &other) const
    {
        return local_aabb.min == other.local_aabb.min &&
               local_aabb.max == other.local_aabb.max &&
               transform == other.transform;
    }
};

// This one uses layermasks
bool shouldCollide(const Collider &a, const Collider &b);

bool checkCollision(const AABB &a, const AABB &b);

glm::vec3 getAABBFixVec(const AABB &a, const AABB &b);