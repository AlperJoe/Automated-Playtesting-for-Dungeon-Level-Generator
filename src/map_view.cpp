#include "map_view.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace {

const ImU32 kFloor  = IM_COL32( 54,  62,  84, 255);
const ImU32 kGrid   = IM_COL32( 45,  52,  72, 255);
const ImU32 kWall   = IM_COL32( 13,  16,  24, 255);
const ImU32 kDoor   = IM_COL32(154, 102,  52, 255);
const ImU32 kPlayer = IM_COL32(124, 197, 118, 255);
const ImU32 kExit   = IM_COL32(179, 146, 240, 255);
const ImU32 kEnemy  = IM_COL32(229,  83,  75, 255);
const ImU32 kItem   = IM_COL32(242, 193,  78, 255);

// Draws one entity symbol centred on c, with r as its half-size.
void draw_entity(ImDrawList* dl, Entity e, ImVec2 c, float r) {
    switch (e) {
    case Entity::Player:
        dl->AddCircleFilled(c, r, kPlayer, 32);
        break;
    case Entity::Spawn:   // the entrance cell next to the player; not drawn
        break;
    case Entity::Exit:
        dl->AddQuadFilled({c.x, c.y - r * 1.1f}, {c.x + r * 1.1f, c.y},
                          {c.x, c.y + r * 1.1f}, {c.x - r * 1.1f, c.y}, kExit);
        break;
    case Entity::Enemy:
        dl->AddTriangleFilled({c.x, c.y - r}, {c.x + r, c.y + r * 0.8f},
                              {c.x - r, c.y + r * 0.8f}, kEnemy);
        break;
    case Entity::Item:
        dl->AddRectFilled({c.x - r * 0.7f, c.y - r * 0.7f}, {c.x + r * 0.7f, c.y + r * 0.7f},
                          kItem, r * 0.25f);
        break;
    case Entity::None:
        break;
    }
}

// The smallest box that holds every non-empty tile, plus one cell around it.
void content_bounds(const Map& map, int& x0, int& y0, int& x1, int& y1) {
    x0 = map.width; y0 = map.height; x1 = -1; y1 = -1;
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x)
            if (map.tile(x, y) != Tile::Empty) {
                x0 = std::min(x0, x); x1 = std::max(x1, x);
                y0 = std::min(y0, y); y1 = std::max(y1, y);
            }
    if (x1 < 0) { x0 = 0; y0 = 0; x1 = map.width - 1; y1 = map.height - 1; return; }
    x0 = std::max(0, x0 - 1); y0 = std::max(0, y0 - 1);
    x1 = std::min(map.width - 1, x1 + 1); y1 = std::min(map.height - 1, y1 + 1);
}

}  // namespace

void draw_map(const Map& map, float height) {
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    if (map.empty()) {
        ImGui::TextDisabled("No map yet.");
        ImGui::Dummy(ImVec2(width, height));
        return;
    }

    int x0, y0, x1, y1;
    content_bounds(map, x0, y0, x1, y1);
    const int cols = x1 - x0 + 1, rows = y1 - y0 + 1;
    const float cell = std::max(2.0f, std::floor(std::min(width / cols, height / rows)));
    const ImVec2 origin(std::floor(start.x + (width - cell * cols) * 0.5f),
                        std::floor(start.y + (height - cell * rows) * 0.5f));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const ImVec2 a(origin.x + (x - x0) * cell, origin.y + (y - y0) * cell);
            const ImVec2 b(a.x + cell, a.y + cell);
            switch (map.tile(x, y)) {
            case Tile::Floor:
                dl->AddRectFilled(a, b, kFloor);
                if (cell >= 8) {   // a faint grid makes cells easy to count
                    dl->AddLine(ImVec2(b.x - 0.5f, a.y), ImVec2(b.x - 0.5f, b.y), kGrid);
                    dl->AddLine(ImVec2(a.x, b.y - 0.5f), ImVec2(b.x, b.y - 0.5f), kGrid);
                }
                break;
            case Tile::Wall:  dl->AddRectFilled(a, b, kWall); break;
            case Tile::Door:  dl->AddRectFilled(a, b, kDoor); break;
            case Tile::Empty: break;
            }
            draw_entity(dl, map.entity(x, y), ImVec2(a.x + cell * 0.5f, a.y + cell * 0.5f), cell * 0.32f);
        }
    }

    ImGui::Dummy(ImVec2(width, height));
}

void draw_legend() {
    struct Row { Entity e; const char* name; };
    const Row rows[] = {
        {Entity::Player, "Player"}, {Entity::Exit, "Exit"},
        {Entity::Enemy, "Enemy"}, {Entity::Item, "Item"},
    };
    const float size = ImGui::GetTextLineHeight();
    bool first = true;
    for (const auto& row : rows) {
        if (!first) ImGui::SameLine(0, size * 1.6f);
        first = false;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        draw_entity(ImGui::GetWindowDrawList(), row.e,
                    ImVec2(p.x + size * 0.5f, p.y + size * 0.5f), size * 0.32f);
        ImGui::Dummy(ImVec2(size, size));
        ImGui::SameLine(0, size * 0.4f);
        ImGui::TextDisabled("%s", row.name);
    }
}
