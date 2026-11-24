#include "map.hpp"
#include <algorithm>

int Map::count(Entity e) const {
    return (int)std::count(entities.begin(), entities.end(), e);
}
