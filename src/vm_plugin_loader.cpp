#include "gambit/vm_plugin_loader.hpp"
#include "gambit/vm_plugin_api.hpp"

#include <stdexcept>
#include <iostream>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <dirent.h>
    #include <sys/stat.h>
#endif

namespace gambit {

VMPluginLoader::VMPluginLoader(VMRegistry& registry)
    : registry_(registry) {}

void VMPluginLoader::loadPlugin(const std::string& path) {
#if defined(_WIN32)
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) {
        std::cerr << "Failed to load plugin: " << path << "\n";
        return;
    }

    auto fn = reinterpret_cast<GambitRegisterVMFn>(
        GetProcAddress(handle, "gambit_register_vm")
    );
    if (!fn) {
        std::cerr << "Plugin missing gambit_register_vm: " << path << "\n";
        return;
    }

    if (!fn(&registry_)) {
        std::cerr << "Plugin gambit_register_vm failed: " << path << "\n";
        return;
    }

#else
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        std::cerr << "Failed to load plugin: " << path
                  << " error: " << dlerror() << "\n";
        return;
    }

    dlerror(); // clear
    auto fn = reinterpret_cast<GambitRegisterVMFn>(
        dlsym(handle, "gambit_register_vm")
    );
    const char* err = dlerror();
    if (err || !fn) {
        std::cerr << "Plugin missing gambit_register_vm: " << path
                  << " err: " << (err ? err : "unknown") << "\n";
        return;
    }

    if (!fn(&registry_)) {
        std::cerr << "Plugin gambit_register_vm failed: " << path << "\n";
        return;
    }
#endif

    std::cout << "Loaded VM plugin: " << path << "\n";
}

void VMPluginLoader::loadFromDirectory(const std::string& dir) {
#if defined(_WIN32)
    std::string pattern = dir + "\\*.dll";
    WIN32_FIND_DATAA data;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string path = dir + "\\" + data.cFileName;
            loadPlugin(path);
        }
    } while (FindNextFileA(hFind, &data));
    FindClose(hFind);

#else
    DIR* dp = opendir(dir.c_str());
    if (!dp) return;

    struct dirent* entry;
    while ((entry = readdir(dp)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;

        std::string path = dir + "/" + name;

        struct stat st;
        if (stat(path.c_str(), &st) != 0) continue;
        if (!S_ISREG(st.st_mode)) continue;

        // crude filter: .so
        if (path.size() >= 3 && path.substr(path.size() - 3) == ".so") {
            loadPlugin(path);
        }
    }
    closedir(dp);
#endif
}

} // namespace gambit
