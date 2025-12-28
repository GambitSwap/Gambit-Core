#pragma once
#include <cstdint>

#include "gambit/vm.hpp"
#include "gambit/vm_registry.hpp"

extern "C" {

// Plugin must export this function:
//
//   bool gambit_register_vm(gambit::VMRegistry* registry);
//
// It should create its VM instance(s) and register them into the registry.
// Returns true on success.
typedef bool (*GambitRegisterVMFn)(gambit::VMRegistry* registry);

} // extern "C"

/*
extern "C" bool gambit_register_vm(gambit::VMRegistry* registry);

Inside that, it calls registry->registerVM(...) with its own VM type and instance.
*/