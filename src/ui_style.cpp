#include "ui_style.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

namespace ui {
namespace {

Fonts g_fonts;

constexpr float kFontSize = 15.0f;

ImVec4 with_alpha(ImVec4 c, float a) { return ImVec4(c.x, c.y, c.z, a); }
ImU32 u32(ImVec4 c) { return ImGui::GetColorU32(c); }

ImFont* load_font(const std::string& path) {
    if (!std::filesystem::exists(path)) return nullptr;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), kFontSize);
}

}  // namespace

const Fonts& fonts() { return g_fonts; }

void setup(const char* assets_dir, float scale) {
    const std::string dir = std::string(assets_dir) + "/fonts/";
    g_fonts.regular = load_font(dir + "Inter-Regular.otf");
    g_fonts.bold    = load_font(dir + "Inter-SemiBold.otf");
    if (!g_fonts.regular) g_fonts.regular = ImGui::GetIO().Fonts->AddFontDefault();
    if (!g_fonts.bold)    g_fonts.bold    = g_fonts.regular;
    ImGui::GetIO().FontDefault = g_fonts.regular;

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding     = ImVec2(22, 22);
    s.FramePadding      = ImVec2(10, 7);
    s.ItemSpacing       = ImVec2(10, 8);
    s.ItemInnerSpacing  = ImVec2(8, 6);
    s.ScrollbarSize     = 10;
    s.WindowRounding    = 0;
    s.ChildRounding     = 12;
    s.FrameRounding     = 7;
    s.PopupRounding     = 8;
    s.GrabRounding      = 7;
    s.ScrollbarRounding = 6;
    s.WindowBorderSize  = 0;
    s.ChildBorderSize   = 0;
    s.FrameBorderSize   = 0;
    s.PopupBorderSize   = 1;
    s.ScaleAllSizes(scale);
    s.FontSizeBase = kFontSize;
    s.FontScaleDpi = scale;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                 = color::text;
    c[ImGuiCol_TextDisabled]         = color::faint;
    c[ImGuiCol_WindowBg]             = color::bg;
    c[ImGuiCol_ChildBg]              = color::panel;
    c[ImGuiCol_PopupBg]              = ImVec4(0.118f, 0.141f, 0.200f, 0.98f);
    c[ImGuiCol_Border]               = with_alpha(color::frame_on, 0.8f);
    c[ImGuiCol_FrameBg]              = color::frame;
    c[ImGuiCol_FrameBgHovered]       = color::frame_hi;
    c[ImGuiCol_FrameBgActive]        = color::frame_on;
    c[ImGuiCol_Button]               = color::frame;
    c[ImGuiCol_ButtonHovered]        = color::frame_hi;
    c[ImGuiCol_ButtonActive]         = color::frame_on;
    c[ImGuiCol_ScrollbarBg]          = with_alpha(color::bg, 0.0f);
    c[ImGuiCol_ScrollbarGrab]        = color::frame_hi;
    c[ImGuiCol_ScrollbarGrabHovered] = color::frame_on;
    c[ImGuiCol_ScrollbarGrabActive]  = color::frame_on;
    c[ImGuiCol_TextSelectedBg]       = with_alpha(color::good, 0.35f);
    c[ImGuiCol_NavCursor]            = color::good;
}

bool primary_button(const char* label, ImVec2 size) {
    ImGui::PushStyleColor(ImGuiCol_Button,        color::good);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.56f, 0.83f, 0.53f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.42f, 0.69f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.06f, 0.09f, 0.06f, 1.0f));
    ImGui::PushFont(fonts().bold, 0.0f);
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopFont();
    ImGui::PopStyleColor(4);
    return pressed;
}

bool subtle_button(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Button, with_alpha(color::frame, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, color::muted);
    const bool pressed = ImGui::Button(label);
    ImGui::PopStyleColor(2);
    return pressed;
}

void help_icon(const char* text) {
    // Centre the icon on the item it explains.
    const float mid_y = (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f;
    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    const float size = ImGui::GetTextLineHeight() * 0.95f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    p.y = mid_y - size * 0.5f;

    ImGui::PushID(text);
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton("help", ImVec2(size, size));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 c(p.x + size * 0.5f, p.y + size * 0.5f);
    const ImU32 col = u32(hovered ? color::good : color::faint);
    dl->AddCircle(c, size * 0.45f, col, 24, 1.2f);
    ImFont* font = fonts().bold;
    const float fs = ImGui::GetFontSize() * 0.72f;
    const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, "?");
    dl->AddText(font, fs, ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), col, "?");

    if (hovered && ImGui::BeginTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool segmented(const char* id, const char* const* labels, int count, int* selected) {
    ImGui::PushID(id);
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = ImGui::GetFrameHeight();
    const float r = ImGui::GetStyle().FrameRounding;
    const float inset = 3.0f * ImGui::GetStyle().FontScaleDpi;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), u32(color::frame), r);

    bool changed = false;
    const float seg = w / count;
    for (int i = 0; i < count; ++i) {
        const ImVec2 a(p.x + seg * i, p.y);
        ImGui::SetCursorScreenPos(a);
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("opt", ImVec2(seg, h)) && *selected != i) {
            *selected = i;
            changed = true;
        }
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();

        const ImVec2 ia(a.x + inset, a.y + inset), ib(a.x + seg - inset, a.y + h - inset);
        if (*selected == i)
            dl->AddRectFilled(ia, ib, u32(color::frame_on), r - inset * 0.5f);
        else if (hovered)
            dl->AddRectFilled(ia, ib, u32(color::frame_hi), r - inset * 0.5f);

        ImFont* font = *selected == i ? fonts().bold : fonts().regular;
        const float fs = ImGui::GetFontSize();
        const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, labels[i]);
        dl->AddText(font, fs, ImVec2(std::floor(a.x + (seg - ts.x) * 0.5f), std::floor(a.y + (h - ts.y) * 0.5f)),
                    u32(*selected == i ? color::text : color::muted), labels[i]);
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(w, h));
    ImGui::PopID();
    return changed;
}

bool slider(const char* id, int* value, int min, int max) {
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = ImGui::GetFrameHeight();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, ImVec2(w, h));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    const float knob = h * 0.30f;
    const float x0 = p.x + knob, x1 = p.x + w - knob;
    bool changed = false;
    if (active && max > min) {
        const float t = std::clamp((ImGui::GetIO().MousePos.x - x0) / (x1 - x0), 0.0f, 1.0f);
        const int v = min + (int)std::lround(t * (max - min));
        if (v != *value) { *value = v; changed = true; }
    }
    if (hovered && !active) {   // arrow keys nudge the value while hovering
        int v = *value;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  v = std::max(min, v - 1);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) v = std::min(max, v + 1);
        if (v != *value) { *value = v; changed = true; }
    }

    const float t = max > min ? (float)(std::clamp(*value, min, max) - min) / (max - min) : 0.0f;
    const float cy = p.y + h * 0.5f, th = std::max(2.0f, h * 0.09f);
    const float kx = x0 + t * (x1 - x0);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(ImVec2(x0, cy - th), ImVec2(x1, cy + th), u32(color::frame_hi), th);
    dl->AddRectFilled(ImVec2(x0, cy - th), ImVec2(kx, cy + th), u32(color::good), th);
    const ImVec4 kc = (active || hovered) ? ImVec4(0.60f, 0.86f, 0.57f, 1.0f) : color::good;
    dl->AddCircleFilled(ImVec2(kx, cy), knob, u32(kc), 32);
    return changed;
}

void status(bool ok, const char* text) {
    const float h = ImGui::GetFrameHeight();
    const float r = h * 0.36f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 c(p.x + r, p.y + h * 0.5f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 ink = IM_COL32(16, 22, 16, 255);
    const float th = std::max(1.5f, r * 0.22f);
    if (ok) {
        dl->AddCircleFilled(c, r, u32(color::good), 32);
        const ImVec2 pts[3] = {ImVec2(c.x - r * 0.45f, c.y + r * 0.02f),
                               ImVec2(c.x - r * 0.12f, c.y + r * 0.35f),
                               ImVec2(c.x + r * 0.48f, c.y - r * 0.32f)};
        dl->AddPolyline(pts, 3, ink, ImDrawFlags_None, th);
    } else {
        dl->AddCircleFilled(c, r, u32(color::bad), 32);
        const float d = r * 0.38f;
        dl->AddLine(ImVec2(c.x - d, c.y - d), ImVec2(c.x + d, c.y + d), ink, th);
        dl->AddLine(ImVec2(c.x - d, c.y + d), ImVec2(c.x + d, c.y - d), ink, th);
    }
    ImGui::Dummy(ImVec2(r * 2, h));
    ImGui::SameLine(0, r * 0.7f);
    ImGui::AlignTextToFramePadding();
    ImGui::PushFont(fonts().bold, 0.0f);
    ImGui::TextColored(ok ? color::good : color::bad, "%s", text);
    ImGui::PopFont();
}

}  // namespace ui
