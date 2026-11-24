#pragma once
#include "map.hpp"
#include "ls.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// One param declared in the script's `params { ... }` block.
struct Param {
    std::string name;
    int  value         = 0;
    int  default_value = 0;
    bool editable      = true;   // false for derived params and non-literal defaults
};

// Wraps one LevelScript file: compiles it, lists its params and turns
// (seed, param values) into a Map. This is the only place that uses LevelScript.
class LevelSource {
public:
    // (Re)reads the file. On a compile error the previous version stays
    // usable and error() says what went wrong. Param values the user already
    // set are kept by name.
    bool load(const std::string& path);

    Map generate(uint64_t seed) const;

    bool ready() const { return entry_ >= 0; }
    const std::string& path() const  { return path_; }
    const std::string& error() const { return error_; }
    std::string warnings() const;    // from the last compile and the last run
    std::vector<Param>& params()     { return params_; }
    void reset_params();
    bool set_param(const std::string& name, int value);
    Param* find_param(const std::string& name);   // nullptr if the script has no such param

private:
    std::string        path_;
    std::string        error_;
    std::string        compile_warnings_;
    std::shared_ptr<std::string> run_warnings_;   // filled by the generator during a run
    ls::generator      gen_;
    int                entry_ = -1;
    std::vector<Param> params_;
};
