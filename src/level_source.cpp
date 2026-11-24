#include "level_source.hpp"
#include "parser.hpp"   // LevelScript's parser, used only to list the params
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {

bool read_file(const fs::path& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::ostringstream text;
    text << file.rdbuf();
    out = text.str();
    return true;
}

// Tag names in roguelike.lvs. Another script with other names only needs
// changes here.
struct TagNames {
    const char* floor  = "level.F";
    const char* open   = "level.O";
    const char* wall   = "level.W";
    const char* door   = "level.D";
    const char* spawn  = "entities.S";
    const char* exit   = "entities.E";
    const char* player = "entities.P";
    const char* enemy  = "entities.N";
    const char* item   = "entities.I";
};

}  // namespace

bool LevelSource::load(const std::string& path) {
    path_ = path;
    std::string source;
    if (!read_file(path, source)) {
        error_ = "Cannot open " + path;
        return false;
    }

    // Module names are paths relative to the script's folder, so a `use`
    // inside the script is resolved from there.
    const fs::path root = fs::path(path).parent_path();
    const std::string name = fs::path(path).filename().generic_string();
    ls::resolver resolve = [root](const std::string& use_path, const std::string& from)
        -> std::optional<ls::module_source> {
        fs::path rel = (fs::path(from).parent_path() / use_path).lexically_normal();
        std::string text;
        if (!read_file(root / rel, text)) return std::nullopt;
        return ls::module_source{rel.generic_string(), text};
    };

    // The generator keeps this handler and also calls it during runs.
    auto sink = std::make_shared<std::string>();
    ls::generator gen = ls::generator::compile(source, name, resolve,
        [sink](const std::string& w) { *sink += w + "\n"; });
    if (!gen) {
        error_ = gen.error();   // keep the previous generator and params
        return false;
    }

    // The public API has no param list, so read it from the parsed file,
    // the same way Bojan's debugger does.
    ls::diagnostics diags;
    std::vector<Param> params;
    if (auto ast = ls::parse(source, name, diags)) {
        for (const auto& p : ast->params) {
            Param param;
            param.name = p.name.text;
            param.editable = !p.is_derived && p.value &&
                             p.value->kind == ls::expr_kind::int_lit;
            if (param.editable) param.default_value = (int)p.value->int_val;
            param.value = param.default_value;
            for (const auto& old : params_)   // keep what the user already set
                if (old.name == param.name && old.editable && param.editable)
                    param.value = old.value;
            params.push_back(param);
        }
    }

    gen_    = std::move(gen);
    entry_  = gen_.sequence("main");
    if (entry_ < 0 && gen_.sequence_count() > 0) entry_ = 0;
    params_ = std::move(params);
    error_.clear();
    compile_warnings_ = *sink;
    sink->clear();
    run_warnings_ = sink;
    return true;
}

std::string LevelSource::warnings() const {
    return compile_warnings_ + (run_warnings_ ? *run_warnings_ : std::string());
}

void LevelSource::reset_params() {
    for (auto& p : params_) p.value = p.default_value;
}

bool LevelSource::set_param(const std::string& name, int value) {
    for (auto& p : params_)
        if (p.name == name && p.editable) { p.value = value; return true; }
    return false;
}

Map LevelSource::generate(uint64_t seed) const {
    Map map;
    if (!ready()) return map;
    if (run_warnings_) run_warnings_->clear();

    std::vector<std::pair<std::string, int>> values;
    for (const auto& p : params_)
        if (p.editable) values.emplace_back(p.name, p.value);

    const ls::level level = gen_.generate(entry_, seed, values);
    map.width  = level.width();
    map.height = level.height();
    map.tiles.assign((size_t)map.width * map.height, Tile::Empty);
    map.entities.assign((size_t)map.width * map.height, Entity::None);

    const TagNames names;
    const ls::grid geo    = level["level"];
    const ls::grid things = level["entities"];
    const int floor = gen_.tag(names.floor) | gen_.tag(names.open);
    const int wall  = gen_.tag(names.wall);
    const int door  = gen_.tag(names.door);
    const std::pair<int, Entity> kinds[] = {
        {gen_.tag(names.spawn),  Entity::Spawn},
        {gen_.tag(names.exit),   Entity::Exit},
        {gen_.tag(names.player), Entity::Player},
        {gen_.tag(names.enemy),  Entity::Enemy},
        {gen_.tag(names.item),   Entity::Item},
    };

    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            const size_t i = (size_t)y * map.width + x;
            if      (geo.has(x, y, door))  map.tiles[i] = Tile::Door;
            else if (geo.has(x, y, wall))  map.tiles[i] = Tile::Wall;
            else if (geo.has(x, y, floor)) map.tiles[i] = Tile::Floor;

            for (const auto& [mask, kind] : kinds)
                if (mask && things.has(x, y, mask)) { map.entities[i] = kind; break; }
        }
    }
    return map;
}
