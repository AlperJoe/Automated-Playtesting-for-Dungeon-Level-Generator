#pragma once
#include "map.hpp"

// Draws the map into the current ImGui window, inside a box of the given
// height and the full available width. The empty border around the level is
// cropped and the map is centred.
void draw_map(const Map& map, float height);

// One row with the map symbols and their names.
void draw_legend();
