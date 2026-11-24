// Playtester: a level settings panel and a map view over a LevelScript generator.
//
//   playtester [script.lvs] [--seed N] [--param name=value ...] [--snapshot out.bmp]
//
// --snapshot renders a few frames, saves the window as a BMP and quits; it is
// for checking the tool without looking at it.

#include "level_source.hpp"
#include "map_view.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <random>
#include <string>
#include <utility>
#include <vector>

#ifndef DEFAULT_SCRIPT
#define DEFAULT_SCRIPT "scripts/roguelike.lvs"
#endif

namespace {

struct App {
    LevelSource source;
    Map         map;
    uint32_t    seed = 42;
    bool        auto_generate = true;

    void generate() { map = source.generate(seed); }
};

uint32_t random_seed() {
    static std::mt19937 rng{std::random_device{}()};
    return rng();
}

void draw_settings(App& app) {
    bool changed = false;

    ImGui::SeparatorText("Script");
    ImGui::TextWrapped("%s", app.source.path().c_str());
    if (ImGui::Button("Reload script")) {
        app.source.load(app.source.path());
        changed = true;
    }
    if (!app.source.error().empty())
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.40f, 1.0f), "%s", app.source.error().c_str());
    const std::string warnings = app.source.warnings();
    if (!warnings.empty())
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.35f, 1.0f), "%s", warnings.c_str());

    ImGui::SeparatorText("Seed");
    changed |= ImGui::InputScalar("##seed", ImGuiDataType_U32, &app.seed);
    ImGui::SameLine();
    if (ImGui::Button("Random")) {
        app.seed = random_seed();
        changed = true;
    }

    ImGui::SeparatorText("Params");
    for (auto& p : app.source.params()) {
        if (!p.editable) {
            ImGui::TextDisabled("%s (computed)", p.name.c_str());
            continue;
        }
        changed |= ImGui::InputInt(p.name.c_str(), &p.value);
        ImGui::SetItemTooltip("Default: %d", p.default_value);
    }
    if (ImGui::Button("Reset to defaults")) {
        app.source.reset_params();
        changed = true;
    }

    ImGui::SeparatorText("Generate");
    ImGui::Checkbox("Generate on every change", &app.auto_generate);
    if (ImGui::Button("Generate") || (changed && app.auto_generate))
        app.generate();

    ImGui::SeparatorText("Map");
    if (!app.map.empty())
        ImGui::Text("%d x %d cells", app.map.width, app.map.height);
    draw_legend(app.map);
}

void draw_ui(App& app, float scale) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("Playtester", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::BeginChild("settings", ImVec2(320.0f * scale, 0), ImGuiChildFlags_Borders);
    draw_settings(app);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("map", ImVec2(0, 0), ImGuiChildFlags_Borders);
    draw_map(app.map);
    ImGui::EndChild();

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
    if (!app.source.load(script))
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
    SDL_Window* window = SDL_CreateWindow("Playtester", (int)(1280 * scale), (int)(800 * scale),
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
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // no imgui.ini next to the tool
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 3.0f;
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;
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
        SDL_SetRenderDrawColor(renderer, 18, 22, 33, 255);
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
