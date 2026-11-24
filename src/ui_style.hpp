#pragma once
#include "imgui.h"

// Look and feel of the tool: colours, fonts and a few small widgets.
// The colours follow the visual plan (dark navy cards, warm white text,
// green / orange / purple accents), so the tool and the plan look related.
namespace ui {

namespace color {
inline const ImVec4 bg       {0.071f, 0.086f, 0.122f, 1.0f};   // #12161F window
inline const ImVec4 panel    {0.102f, 0.122f, 0.173f, 1.0f};   // #1A1F2C settings card
inline const ImVec4 canvas   {0.086f, 0.106f, 0.153f, 1.0f};   // #161B27 map card
inline const ImVec4 frame    {0.137f, 0.165f, 0.235f, 1.0f};   // #232A3C inputs
inline const ImVec4 frame_hi {0.173f, 0.208f, 0.290f, 1.0f};   // #2C354A hover
inline const ImVec4 frame_on {0.208f, 0.247f, 0.345f, 1.0f};   // #353F58 active
inline const ImVec4 text     {0.945f, 0.922f, 0.867f, 1.0f};   // #F1EBDD
inline const ImVec4 muted    {0.604f, 0.627f, 0.682f, 1.0f};   // #9AA0AE
inline const ImVec4 faint    {0.420f, 0.443f, 0.518f, 1.0f};   // #6B7184
inline const ImVec4 good     {0.486f, 0.773f, 0.463f, 1.0f};   // #7CC576
inline const ImVec4 warn     {0.949f, 0.690f, 0.290f, 1.0f};   // #F2B04A
inline const ImVec4 bad      {0.898f, 0.325f, 0.294f, 1.0f};   // #E5534B
}

struct Fonts {
    ImFont* regular = nullptr;
    ImFont* bold    = nullptr;
};
const Fonts& fonts();

// Loads Inter from assets_dir/fonts (falls back to ImGui's built-in font)
// and applies the colours and spacing. scale is the display content scale.
void setup(const char* assets_dir, float scale);

// Big green button for the main action.
bool primary_button(const char* label, ImVec2 size);

// Quiet text button for secondary actions.
bool subtle_button(const char* label);

// Small round "?" after the last item. Hovering it shows the text.
void help_icon(const char* text);

// A row of options where exactly one is picked, e.g. Small / Medium / Large.
// selected is -1 when none of the options matches. Returns true on a change.
bool segmented(const char* id, const char* const* labels, int count, int* selected);

// A thin track with a round knob, filling the available width.
bool slider(const char* id, int* value, int min, int max);

// A green tick or a red cross in a circle, followed by a short text.
void status(bool ok, const char* text);

}  // namespace ui
