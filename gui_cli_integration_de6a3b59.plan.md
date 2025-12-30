---
name: GUI CLI Integration
overview: Consolidate the GUI into the main executable with a --gui flag while also supporting RPC-based connection to remote nodes. The GUI will no longer duplicate core component initialization.
todos:
  - id: create-node-class
    content: Create Node class in include/gambit/node.hpp and src/node.cpp to encapsulate core components
    status: completed
  - id: refactor-main-cpp
    content: Refactor app/main.cpp to use Node class and add --gui flag handling
    status: completed
    dependencies:
      - create-node-class
  - id: refactor-gui
    content: Refactor gui/main_gui.cpp to accept Node* or RPC URL instead of creating own components
    status: completed
    dependencies:
      - create-node-class
  - id: add-rpc-client
    content: Add RPC client mode to GUI for connecting to remote nodes
    status: completed
  - id: update-cmake
    content: Update CMake to link FLTK to main executable when GUI enabled
    status: completed
    dependencies:
      - refactor-main-cpp
      - refactor-gui
---

# Unified GUI/CLI Node with RPC Support

## Current Problem

- [`gui/main_gui.cpp`](gui/main_gui.cpp) duplicates all blockchain initialization from [`app/main.cpp`](app/main.cpp)
- Two separate executables that don't share code
- `NodeConfig.enableGUI` exists but is unused

## Architecture

```mermaid
flowchart TB
    subgraph SingleExe [gambit_node executable]
        Main[main.cpp] --> Config[NodeConfig]
        Config --> |--gui flag| GUIMode[GUI Mode]
        Config --> |no flag| CLIMode[CLI Mode]
        
        subgraph SharedCore [Shared Core Components]
            Blockchain
            P2PNode
            RpcServer
            Miner
        end
        
        GUIMode --> SharedCore
        CLIMode --> SharedCore
        GUIMode --> FLTKLoop[FLTK Event Loop]
        CLIMode --> BlockingLoop[Blocking Loop]
    end
    
    subgraph RemoteMode [Optional: Remote GUI]
        StandaloneGUI[gambit_gui.exe] --> |RPC calls| ExistingNode[Running Node]
    end
```



## Implementation Plan

### 1. Create a shared Node class

Extract the core node setup from `main.cpp` into a reusable `Node` class in [`include/gambit/node.hpp`](include/gambit/node.hpp):

- Holds Blockchain, P2PNode, RpcServer, Miner instances
- Accepts `NodeConfig` for configuration
- Provides accessors for GUI to use

### 2. Refactor GUI to use Node class

Modify [`gui/main_gui.cpp`](gui/main_gui.cpp):

- Remove duplicated blockchain/p2p/miner initialization
- Accept `Node*` reference or connect via RPC URL
- Two modes: **embedded** (uses Node directly) or **remote** (connects via RPC)

### 3. Integrate GUI into main executable

Update [`app/main.cpp`](app/main.cpp):

- Wire up the existing `--gui` flag parsing
- When `enableGUI=true`, launch GUI with shared Node instance
- FLTK event loop replaces the blocking `while(true)` loop

### 4. Keep standalone GUI option

Update [`gui/main_gui.cpp`](gui/main_gui.cpp) to support RPC mode: