// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vpicorv32.h for the primary calling header

#include "Vpicorv32__pch.h"
#include "Vpicorv32__Syms.h"
#include "Vpicorv32___024root.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vpicorv32___024root___dump_triggers__stl(Vpicorv32___024root* vlSelf);
#endif  // VL_DEBUG

VL_ATTR_COLD void Vpicorv32___024root___eval_triggers__stl(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___eval_triggers__stl\n"); );
    // Body
    vlSelf->__VstlTriggered.set(0U, (IData)(vlSelf->__VstlFirstIteration));
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vpicorv32___024root___dump_triggers__stl(vlSelf);
    }
#endif
}

VL_ATTR_COLD void Vpicorv32___024root___stl_sequent__TOP__0(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___stl_sequent__TOP__0\n"); );
    // Body
    vlSelf->picorv32_axi__02Etrace_data = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.trace_data;
    vlSelf->picorv32_axi__02Etrace_valid = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.__PVT__do_waitirq;
    vlSelf->picorv32_axi__02Eeoi = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.eoi;
    vlSelf->picorv32_axi__02Epcpi_rs2 = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.__PVT__reg_op2;
    vlSelf->picorv32_axi__02Epcpi_rs1 = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.__PVT__reg_op1;
    vlSelf->picorv32_axi__02Epcpi_insn = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.pcpi_insn;
    vlSelf->picorv32_axi__02Epcpi_valid = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.__PVT__irq_active;
    vlSelf->picorv32_axi__02Etrap = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.trap;
    vlSelf->picorv32_wb__02Etrace_data = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.trace_data;
    vlSelf->picorv32_wb__02Etrace_valid = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.__PVT__do_waitirq;
    vlSelf->picorv32_wb__02Eeoi = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.eoi;
    vlSelf->picorv32_wb__02Epcpi_rs2 = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.__PVT__reg_op2;
    vlSelf->picorv32_wb__02Epcpi_rs1 = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.__PVT__reg_op1;
    vlSelf->picorv32_wb__02Epcpi_insn = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.pcpi_insn;
    vlSelf->picorv32_wb__02Epcpi_valid = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.__PVT__irq_active;
    vlSelf->mem_instr = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.mem_instr;
    vlSelf->picorv32_wb__02Etrap = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.trap;
    vlSelf->mem_axi_wstrb = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_wstrb;
    vlSelf->mem_axi_wdata = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_wdata;
    vlSelf->mem_axi_arprot = ((IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_instr)
                               ? 4U : 0U);
    vlSelf->rdata1 = ((0x1eU >= (0x1fU & (~ (IData)(vlSelf->raddr1))))
                       ? vlSelf->picorv32_regs__DOT__regs
                      [(0x1fU & (~ (IData)(vlSelf->raddr1)))]
                       : 0U);
    vlSelf->rdata2 = ((0x1eU >= (0x1fU & (~ (IData)(vlSelf->raddr2))))
                       ? vlSelf->picorv32_regs__DOT__regs
                      [(0x1fU & (~ (IData)(vlSelf->raddr2)))]
                       : 0U);
    vlSelf->mem_axi_awaddr = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_addr;
    vlSelf->mem_axi_rready = ((~ (IData)((0U != (IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_wstrb)))) 
                              & (IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_valid));
    vlSelf->mem_axi_bready = ((IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_valid) 
                              & (0U != (IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_wstrb)));
    vlSelf->picorv32_axi__DOT__mem_ready = ((IData)(vlSelf->mem_axi_bvalid) 
                                            | (IData)(vlSelf->mem_axi_rvalid));
    vlSelf->mem_axi_araddr = vlSelf->mem_axi_awaddr;
    vlSelf->mem_axi_arvalid = ((~ (IData)(vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_arvalid)) 
                               & (IData)(vlSelf->mem_axi_rready));
    vlSelf->mem_axi_awvalid = ((~ (IData)(vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_awvalid)) 
                               & (IData)(vlSelf->mem_axi_bready));
    vlSelf->mem_axi_wvalid = ((~ (IData)(vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_wvalid)) 
                              & (IData)(vlSelf->mem_axi_bready));
}

VL_ATTR_COLD void Vpicorv32_picorv32__pi1___stl_sequent__TOP__picorv32_axi__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf);
VL_ATTR_COLD void Vpicorv32_picorv32__pi1___stl_sequent__TOP__picorv32_wb__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf);

VL_ATTR_COLD void Vpicorv32___024root___eval_stl(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___eval_stl\n"); );
    // Body
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        Vpicorv32___024root___stl_sequent__TOP__0(vlSelf);
        Vpicorv32_picorv32__pi1___stl_sequent__TOP__picorv32_axi__DOT__picorv32_core__0((&vlSymsp->TOP__picorv32_axi__DOT__picorv32_core));
        Vpicorv32_picorv32__pi1___stl_sequent__TOP__picorv32_wb__DOT__picorv32_core__0((&vlSymsp->TOP__picorv32_wb__DOT__picorv32_core));
    }
}
