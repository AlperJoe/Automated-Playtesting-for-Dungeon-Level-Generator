#include "level_check.hpp"
#include <queue>
#include <vector>

LevelCheck check_level(const Map& map) {
    LevelCheck check;
    if (map.empty()) return check;

    int start = -1;
    for (int i = 0; i < (int)map.entities.size(); ++i) {
        if (map.entities[i] == Entity::Player) start = i;
        if (map.entities[i] == Entity::Exit) check.has_exit = true;
    }
    check.has_player = start >= 0;

    // Breadth-first walk from the player.
    std::vector<bool> reached(map.tiles.size(), false);
    if (check.has_player) {
        std::queue<int> open;
        open.push(start);
        reached[start] = true;
        const int dx[] = {1, -1, 0, 0};
        const int dy[] = {0, 0, 1, -1};
        while (!open.empty()) {
            const int i = open.front();
            open.pop();
            const int x = i % map.width;
            const int y = i / map.width;
            for (int d = 0; d < 4; ++d) {
                const int nx = x + dx[d];
                const int ny = y + dy[d];
                if (nx < 0 || ny < 0 || nx >= map.width || ny >= map.height) continue;
                const int n = ny * map.width + nx;
                const Tile t = map.tiles[n];
                if (reached[n] || (t != Tile::Floor && t != Tile::Door)) continue;
                reached[n] = true;
                open.push(n);
            }
        }
    }

    if (!check.has_player) return check;   // nothing to walk from
    for (int i = 0; i < (int)map.entities.size(); ++i) {
        switch (map.entities[i]) {
        case Entity::Exit:  if (reached[i]) check.exit_reachable = true; break;
        case Entity::Item:  if (!reached[i]) ++check.unreachable_items; break;
        case Entity::Enemy: if (!reached[i]) ++check.unreachable_enemies; break;
        default: break;
        }
    }
    return check;
}
