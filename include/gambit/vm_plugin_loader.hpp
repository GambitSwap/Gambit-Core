#pragma once
#include <string>
#include <vector>

#include "gambit/vm_registry.hpp"

namespace gambit {

class VMPluginLoader {
public:
    explicit VMPluginLoader(VMRegistry& registry);

    // Load all plugins from a directory (e.g. "plugins/")
    void loadFromDirectory(const std::string& dir);

    // Load single plugin (full path to .so/.dll)
    void loadPlugin(const std::string& path);

private:
    VMRegistry& registry_;
};

} // namespace gambit
