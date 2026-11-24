#include "map_view.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace {

const ImU32 kFloor  = IM_COL32( 58,  66,  88, 255);
const ImU32 kWall   = IM_COL32( 22,  26,  37, 255);
const ImU32 kDoor   = IM_COL32(138,  90,  43, 255);
const ImU32 kPlayer = IM_COL32(124, 197, 118, 255);
const ImU32 kExit   = IM_COL32(179, 146, 240, 255);
const ImU32 kEnemy  = IM_COL32(229,  83,  75, 255);
const ImU32 kItem   = IM_COL32(242, 193,  78, 255);

// Draws one entity symbol centred on c, with r as its half-size.
void draw_entity(ImDrawList* dl, Entity e, ImVec2 c, float r) {
    switch (e) {
    case Entity::Player:
        dl->AddCircleFilled(c, r, kPlayer);
        break;
    case Entity::Spawn:
        dl->AddCircle(c, r * 0.8f, kPlayer, 0, std::max(1.0f, r * 0.25f));
        break;
    case Entity::Exit:
        dl->AddQuadFilled({c.x, c.y - r}, {c.x + r, c.y}, {c.x, c.y + r}, {c.x - r, c.y}, kExit);
        break;
    case Entity::Enemy:
        dl->AddTriangleFilled({c.x, c.y - r}, {c.x + r, c.y + r * 0.8f}, {c.x - r, c.y + r * 0.8f}, kEnemy);
        break;
    case Entity::Item:
        dl->AddRectFilled({c.x - r * 0.7f, c.y - r * 0.7f}, {c.x + r * 0.7f, c.y + r * 0.7f}, kItem);
        break;
    case Entity::None:
        break;
    }
}

}  // namespace

void draw_map(const Map& map) {
    if (map.empty()) {
        ImGui::TextDisabled("No map yet.");
        return;
    }
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float cell = std::max(2.0f, std::floor(std::min(avail.x / map.width,
                                                          avail.y / map.height)));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            const ImVec2 a{origin.x + x * cell, origin.y + y * cell};
            const ImVec2 b{a.x + cell, a.y + cell};
            switch (map.tile(x, y)) {
            case Tile::Floor: dl->AddRectFilled(a, b, kFloor); break;
            case Tile::Wall:  dl->AddRectFilled(a, b, kWall);  break;
            case Tile::Door:  dl->AddRectFilled(a, b, kDoor);  break;
            case Tile::Empty: break;
            }
            const ImVec2 c{a.x + cell * 0.5f, a.y + cell * 0.5f};
            draw_entity(dl, map.entity(x, y), c, cell * 0.32f);
        }
    }
    ImGui::Dummy(ImVec2(cell * map.width, cell * map.height));   // reserve the space
}

void draw_legend(const Map& map) {
    struct Row { Entity e; const char* name; };
    const Row rows[] = {
        {Entity::Player, "Player"}, {Entity::Spawn, "Spawn"}, {Entity::Exit, "Exit"},
        {Entity::Enemy, "Enemies"}, {Entity::Item, "Items"},
    };
    const float size = ImGui::GetTextLineHeight();
    for (const auto& row : rows) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        draw_entity(ImGui::GetWindowDrawList(), row.e,
                    {p.x + size * 0.5f, p.y + size * 0.5f}, size * 0.35f);
        ImGui::Dummy(ImVec2(size, size));
        ImGui::SameLine();
        ImGui::Text("%s  %d", row.name, map.count(row.e));
    }
}
