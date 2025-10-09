// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef VERILATED_VPICORV32__SYMS_H_
#define VERILATED_VPICORV32__SYMS_H_  // guard

#include "verilated.h"

// INCLUDE MODEL CLASS

#include "Vpicorv32.h"

// INCLUDE MODULE CLASSES
#include "Vpicorv32___024root.h"
#include "Vpicorv32_picorv32__pi1.h"

// SYMS CLASS (contains all model state)
class alignas(VL_CACHE_LINE_BYTES)Vpicorv32__Syms final : public VerilatedSyms {
  public:
    // INTERNAL STATE
    Vpicorv32* const __Vm_modelp;
    VlDeleter __Vm_deleter;
    bool __Vm_didInit = false;

    // MODULE INSTANCE STATE
    Vpicorv32___024root            TOP;
    Vpicorv32_picorv32__pi1        TOP__picorv32_axi__DOT__picorv32_core;
    Vpicorv32_picorv32__pi1        TOP__picorv32_wb__DOT__picorv32_core;

    // CONSTRUCTORS
    Vpicorv32__Syms(VerilatedContext* contextp, const char* namep, Vpicorv32* modelp);
    ~Vpicorv32__Syms();

    // METHODS
    const char* name() { return TOP.name(); }
};

#endif  // guard
