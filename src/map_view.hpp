#pragma once
#include "map.hpp"

// Draws the map into the current ImGui window, scaled to fit the space left.
void draw_map(const Map& map);

// A one-line key for the entity symbols, with how many of each the map has.
void draw_legend(const Map& map);
