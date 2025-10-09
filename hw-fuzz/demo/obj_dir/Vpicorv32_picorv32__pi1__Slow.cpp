// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vpicorv32.h for the primary calling header

#include "Vpicorv32__pch.h"
#include "Vpicorv32__Syms.h"
#include "Vpicorv32_picorv32__pi1.h"

void Vpicorv32_picorv32__pi1___ctor_var_reset(Vpicorv32_picorv32__pi1* vlSelf);

Vpicorv32_picorv32__pi1::Vpicorv32_picorv32__pi1(Vpicorv32__Syms* symsp, const char* v__name)
    : VerilatedModule{v__name}
    , vlSymsp{symsp}
 {
    // Reset structure values
    Vpicorv32_picorv32__pi1___ctor_var_reset(this);
}

void Vpicorv32_picorv32__pi1::__Vconfigure(bool first) {
    if (false && first) {}  // Prevent unused
}

Vpicorv32_picorv32__pi1::~Vpicorv32_picorv32__pi1() {
}
