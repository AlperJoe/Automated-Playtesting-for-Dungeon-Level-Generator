// Alper's Playtester: build dungeon levels with a LevelScript generator and
// look at them.
//
//   playtester [script.lvs] [--seed N] [--param name=value ...] [--snapshot out.bmp]
//
// --snapshot renders a few frames, saves the window as a BMP and quits; it is
// for checking the tool without looking at it.

#include "level_check.hpp"
#include "level_source.hpp"
#include "map_view.hpp"
#include "ui_style.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifndef DEFAULT_SCRIPT
#define DEFAULT_SCRIPT "scripts/roguelike.lvs"
#endif
#ifndef ASSET_DIR
#define ASSET_DIR "assets"
#endif

namespace fs = std::filesystem;

namespace {

const char* const kTitle = "Alper's Playtester";

// ── The four controls ────────────────────────────────────────────────────────
// Size and Rooms only offer values that were tested: every combination gave a
// playable level in 200 out of 200 seeds. Enemies and Items never made a level
// unplayable in tests, so they are free sliders.

struct Option {
    const char* label;
    std::vector<std::pair<const char*, int>> values;   // param name and value
};

struct Choice {
    const char* label;
    std::vector<Option> options;
};

struct Amount {
    const char* label;
    const char* help;
    const char* param;
    int max;
    Entity counted;
};

const Choice kSize{"Size", {
    {"Small",  {{"map_width", 24}, {"map_height", 16}}},
    {"Medium", {{"map_width", 32}, {"map_height", 20}}},
    {"Large",  {{"map_width", 40}, {"map_height", 24}}},
}};

const Choice kRooms{"Rooms", {
    {"Small",  {{"min_room_size", 4}}},
    {"Medium", {{"min_room_size", 5}}},
    {"Large",  {{"min_room_size", 6}}},
}};

const Amount kEnemies{"Enemies", "How many enemies the level has.\nThe number is the count on this map.",
                      "enemy_percent", 30, Entity::Enemy};
const Amount kItems{"Items", "How many items the level has.\nItems are placed along the walls.",
                    "item_percent", 30, Entity::Item};

// ── App state ────────────────────────────────────────────────────────────────

uint32_t random_seed() {
    static std::mt19937 rng{std::random_device{}()};
    return rng() % 100000;   // short IDs are easier to read and type
}

struct App {
    LevelSource     source;
    Map             map;
    LevelCheck      check;
    uint32_t        seed = 42;
    fs::file_time_type script_time{};
    double          next_file_check = 0.0;

    void generate() {
        map   = source.generate(seed);
        check = check_level(map);
    }

    void load(const std::string& path) {
        source.load(path);
        std::error_code ec;
        script_time = fs::last_write_time(path, ec);
    }

    // Reloads the script when its file changes, so edits show up without a
    // Reload button. Checked twice a second.
    void watch_script() {
        const double now = ImGui::GetTime();
        if (now < next_file_check) return;
        next_file_check = now + 0.5;
        std::error_code ec;
        const auto t = fs::last_write_time(source.path(), ec);
        if (!ec && t != script_time) {
            load(source.path());
            generate();
        }
    }
};

// Index of the option that matches the current params, or -1.
int current_option(LevelSource& src, const Choice& choice) {
    for (int i = 0; i < (int)choice.options.size(); ++i) {
        bool match = true;
        for (const auto& [name, value] : choice.options[i].values) {
            const Param* p = src.find_param(name);
            if (!p || p->value != value) match = false;
        }
        if (match) return i;
    }
    return -1;
}

bool has_params(LevelSource& src, const Choice& choice) {
    for (const auto& [name, value] : choice.options.front().values)
        if (!src.find_param(name)) return false;
    return true;
}

// ── Left panel ───────────────────────────────────────────────────────────────

void label(const char* text) {
    ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.35f));
    ImGui::TextUnformatted(text);
}

bool draw_choice(App& app, const Choice& choice) {
    if (!has_params(app.source, choice)) return false;
    label(choice.label);
    std::vector<const char*> labels;
    for (const auto& o : choice.options) labels.push_back(o.label);
    int selected = current_option(app.source, choice);
    if (!ui::segmented(choice.label, labels.data(), (int)labels.size(), &selected)) return false;
    for (const auto& [name, value] : choice.options[selected].values)
        app.source.set_param(name, value);
    return true;
}

bool draw_amount(App& app, const Amount& amount) {
    Param* p = app.source.find_param(amount.param);
    if (!p) return false;
    label(amount.label);
    ImGui::SetItemTooltip("%s", amount.help);

    // The count on the current map, right-aligned on the label's line.
    char count[16];
    std::snprintf(count, sizeof(count), "%d", app.map.count(amount.counted));
    ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(count).x);
    ImGui::PushFont(ui::fonts().bold, 0.0f);
    ImGui::TextUnformatted(count);
    ImGui::PopFont();

    return ui::slider(amount.param, &p->value, 0, amount.max);
}

void draw_settings(App& app) {
    bool changed = false;

    ImGui::PushFont(ui::fonts().bold, ImGui::GetStyle().FontSizeBase * 1.45f);
    ImGui::TextUnformatted(kTitle);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.6f));

    const float w = ImGui::GetContentRegionAvail().x;
    if (ui::primary_button("Generate new map", ImVec2(w, ImGui::GetFrameHeight() * 1.5f))) {
        app.seed = random_seed();
        changed = true;
    }

    // The map's ID (its seed), directly under the button that changes it.
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("ID");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0f);
    changed |= ImGui::InputScalar("##id", ImGuiDataType_U32, &app.seed);
    ui::help_icon("Every map has an ID. Type an ID to get the same map back.");

    ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.5f));
    ImGui::Separator();

    changed |= draw_choice(app, kSize);
    changed |= draw_choice(app, kRooms);
    changed |= draw_amount(app, kEnemies);
    changed |= draw_amount(app, kItems);

    ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.4f));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().FramePadding.x);
    if (ui::subtle_button("Reset settings")) {
        app.source.reset_params();
        changed = true;
    }

    // Script problems are shown only when there is one.
    if (!app.source.error().empty() || !app.source.warnings().empty()) {
        ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.5f));
        ImGui::PushTextWrapPos(0.0f);
        if (!app.source.error().empty())
            ImGui::TextColored(ui::color::bad, "Script error: %s", app.source.error().c_str());
        if (!app.source.warnings().empty())
            ImGui::TextColored(ui::color::warn, "%s", app.source.warnings().c_str());
        ImGui::PopTextWrapPos();
    }

    if (changed) app.generate();
}

// ── Map pane ─────────────────────────────────────────────────────────────────

void draw_canvas(App& app) {
    const LevelCheck& c = app.check;
    if (c.playable()) {
        ui::status(true, "Playable");
        ImGui::SetItemTooltip("The player can reach the exit, every item and every enemy.");
    } else if (!app.map.empty()) {
        std::string why;
        if (!c.has_player)      why = "no player";
        else if (!c.has_exit)   why = "no exit";
        else if (!c.exit_reachable) why = "the exit cannot be reached";
        else                    why = "some items or enemies cannot be reached";
        ui::status(false, ("Not playable: " + why).c_str());
    }

    const float legend_h = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y * 2;
    draw_map(app.map, std::max(50.0f, ImGui::GetContentRegionAvail().y - legend_h));
    draw_legend();
}

void draw_ui(App& app, float scale) {
    app.watch_script();
    if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_G, false)) {
        app.seed = random_seed();
        app.generate();
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * scale, 12 * scale));
    ImGui::Begin(kTitle, nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();

    ImGui::BeginChild("settings", ImVec2(320.0f * scale, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    draw_settings(app);
    ImGui::EndChild();

    ImGui::SameLine(0, 12 * scale);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ui::color::canvas);
    ImGui::BeginChild("canvas", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    draw_canvas(app);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
}

}  // namespace

int main(int argc, char** argv) {
    std::string script = DEFAULT_SCRIPT;
    std::string snapshot;
    uint32_t seed = 42;
    std::vector<std::pair<std::string, int>> param_args;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--seed" && i + 1 < argc) {
            seed = (uint32_t)std::stoul(argv[++i]);
        } else if (arg == "--snapshot" && i + 1 < argc) {
            snapshot = argv[++i];
        } else if (arg == "--param" && i + 1 < argc) {
            const std::string kv = argv[++i];
            const size_t eq = kv.find('=');
            if (eq != std::string::npos)
                param_args.emplace_back(kv.substr(0, eq), std::stoi(kv.substr(eq + 1)));
        } else {
            script = arg;
        }
    }

    App app;
    app.seed = seed;
    app.load(script);
    if (!app.source.error().empty())
        std::fprintf(stderr, "%s\n", app.source.error().c_str());
    for (const auto& [name, value] : param_args)
        if (!app.source.set_param(name, value))
            std::fprintf(stderr, "Unknown param: %s\n", name.c_str());
    app.generate();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    const float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_Window* window = SDL_CreateWindow(kTitle, (int)(1280 * scale), (int)(820 * scale),
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowMinimumSize(window, (int)(900 * scale), (int)(600 * scale));
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // no imgui.ini next to the tool
    ui::setup(ASSET_DIR, scale);
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool done = false;
    int frame = 0;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        draw_ui(app, scale);
        ImGui::Render();

        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 18, 22, 31, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        if (!snapshot.empty() && ++frame == 3) {
            if (SDL_Surface* shot = SDL_RenderReadPixels(renderer, nullptr)) {
                if (!SDL_SaveBMP(shot, snapshot.c_str()))
                    std::fprintf(stderr, "SDL_SaveBMP failed: %s\n", SDL_GetError());
                SDL_DestroySurface(shot);
            }
            done = true;
        }
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
