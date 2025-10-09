// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table implementation internals

#include "Vpicorv32__pch.h"
#include "Vpicorv32.h"
#include "Vpicorv32___024root.h"
#include "Vpicorv32_picorv32__pi1.h"

// FUNCTIONS
Vpicorv32__Syms::~Vpicorv32__Syms()
{
}

Vpicorv32__Syms::Vpicorv32__Syms(VerilatedContext* contextp, const char* namep, Vpicorv32* modelp)
    : VerilatedSyms{contextp}
    // Setup internal state of the Syms class
    , __Vm_modelp{modelp}
    // Setup module instances
    , TOP{this, namep}
    , TOP__picorv32_axi__DOT__picorv32_core{this, Verilated::catName(namep, "picorv32_axi.picorv32_core")}
    , TOP__picorv32_wb__DOT__picorv32_core{this, Verilated::catName(namep, "picorv32_wb.picorv32_core")}
{
    // Configure time unit / time precision
    _vm_contextp__->timeunit(-9);
    _vm_contextp__->timeprecision(-12);
    // Setup each module's pointers to their submodules
    TOP.__PVT__picorv32_axi__DOT__picorv32_core = &TOP__picorv32_axi__DOT__picorv32_core;
    TOP.__PVT__picorv32_wb__DOT__picorv32_core = &TOP__picorv32_wb__DOT__picorv32_core;
    // Setup each module's pointer back to symbol table (for public functions)
    TOP.__Vconfigure(true);
    TOP__picorv32_axi__DOT__picorv32_core.__Vconfigure(true);
    TOP__picorv32_wb__DOT__picorv32_core.__Vconfigure(false);
}
