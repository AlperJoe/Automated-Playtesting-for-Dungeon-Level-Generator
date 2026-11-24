#pragma once
#include "map.hpp"

// Checks that a generated level can be played at all, before anyone plays it.
// It follows the first check in Liapis et al. (2015): every object must be
// reachable. Their second check (a basic agent survives the monsters) needs
// the game rules and comes with step 2.
struct LevelCheck {
    bool has_player          = false;
    bool has_exit            = false;
    bool exit_reachable      = false;
    int  unreachable_items   = 0;
    int  unreachable_enemies = 0;

    bool playable() const {
        return has_player && has_exit && exit_reachable &&
               unreachable_items == 0 && unreachable_enemies == 0;
    }
};

// Walks from the player over floor and door cells, in four directions.
// Items and enemies stand on floor cells, so they do not block the walk.
// Without a player there is nothing to walk from, so only has_player and
// has_exit are filled in.
LevelCheck check_level(const Map& map);
