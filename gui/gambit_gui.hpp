#pragma once

#include "gambit/node.hpp"

namespace gambit {

// Run GUI in embedded mode - uses the node directly
void runGUI(Node& node);

// Run GUI in RPC client mode - connects to a remote node
void runGUIWithRPC(const std::string& rpcUrl);

} // namespace gambit

