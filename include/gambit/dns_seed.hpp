#pragma once
#include <vector>
#include <string>

namespace gambit {

class DNSSeedManager {
public:
    DNSSeedManager();

    void addSeed(const std::string& domain);
    std::vector<std::string> resolveAll() const;

private:
    std::vector<std::string> seeds_;
};

} // namespace gambit
