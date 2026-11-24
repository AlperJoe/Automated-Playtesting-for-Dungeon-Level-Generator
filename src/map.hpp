#pragma once
#include <vector>

// The tool's own copy of a generated level.
// Everything after generation (the game in step 2, the agents later) reads a
// Map and never talks to LevelScript directly. That keeps the generator
// replaceable and the rest of the code simple.

enum class Tile : unsigned char { Empty, Floor, Wall, Door };

enum class Entity : unsigned char { None, Spawn, Exit, Player, Enemy, Item };

struct Map {
    int width  = 0;
    int height = 0;
    std::vector<Tile>   tiles;      // width * height, row by row
    std::vector<Entity> entities;   // width * height, row by row

    bool empty() const { return width == 0 || height == 0; }
    Tile   tile(int x, int y) const   { return tiles[y * width + x]; }
    Entity entity(int x, int y) const { return entities[y * width + x]; }

    int count(Entity e) const;      // how many cells hold this entity
};
