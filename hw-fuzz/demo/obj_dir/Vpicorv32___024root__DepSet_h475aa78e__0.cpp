// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vpicorv32.h for the primary calling header

#include "Vpicorv32__pch.h"
#include "Vpicorv32__Syms.h"
#include "Vpicorv32___024root.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vpicorv32___024root___dump_triggers__ico(Vpicorv32___024root* vlSelf);
#endif  // VL_DEBUG

void Vpicorv32___024root___eval_triggers__ico(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___eval_triggers__ico\n"); );
    // Body
    vlSelf->__VicoTriggered.set(0U, (IData)(vlSelf->__VicoFirstIteration));
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vpicorv32___024root___dump_triggers__ico(vlSelf);
    }
#endif
}

void Vpicorv32___024root___ico_sequent__TOP__0(Vpicorv32___024root* vlSelf);
void Vpicorv32_picorv32__pi1___ico_sequent__TOP__picorv32_axi__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf);
void Vpicorv32_picorv32__pi1___ico_sequent__TOP__picorv32_wb__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf);

void Vpicorv32___024root___eval_ico(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___eval_ico\n"); );
    // Body
    if ((1ULL & vlSelf->__VicoTriggered.word(0U))) {
        Vpicorv32___024root___ico_sequent__TOP__0(vlSelf);
        Vpicorv32_picorv32__pi1___ico_sequent__TOP__picorv32_axi__DOT__picorv32_core__0((&vlSymsp->TOP__picorv32_axi__DOT__picorv32_core));
        Vpicorv32_picorv32__pi1___ico_sequent__TOP__picorv32_wb__DOT__picorv32_core__0((&vlSymsp->TOP__picorv32_wb__DOT__picorv32_core));
    }
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vpicorv32___024root___dump_triggers__act(Vpicorv32___024root* vlSelf);
#endif  // VL_DEBUG

void Vpicorv32___024root___eval_triggers__act(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___eval_triggers__act\n"); );
    // Body
    vlSelf->__VactTriggered.set(0U, ((IData)(vlSelf->picorv32_regs__02Eclk) 
                                     & (~ (IData)(vlSelf->__Vtrigprevexpr___TOP__picorv32_regs__02Eclk__0))));
    vlSelf->__VactTriggered.set(1U, ((IData)(vlSelf->picorv32_axi__02Eclk) 
                                     & (~ (IData)(vlSelf->__Vtrigprevexpr___TOP__picorv32_axi__02Eclk__0))));
    vlSelf->__VactTriggered.set(2U, ((IData)(vlSelf->wb_clk_i) 
                                     & (~ (IData)(vlSelf->__Vtrigprevexpr___TOP__wb_clk_i__0))));
    vlSelf->__Vtrigprevexpr___TOP__picorv32_regs__02Eclk__0 
        = vlSelf->picorv32_regs__02Eclk;
    vlSelf->__Vtrigprevexpr___TOP__picorv32_axi__02Eclk__0 
        = vlSelf->picorv32_axi__02Eclk;
    vlSelf->__Vtrigprevexpr___TOP__wb_clk_i__0 = vlSelf->wb_clk_i;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vpicorv32___024root___dump_triggers__act(vlSelf);
    }
#endif
}

VL_INLINE_OPT void Vpicorv32___024root___nba_sequent__TOP__1(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___nba_sequent__TOP__1\n"); );
    // Body
    vlSelf->picorv32_axi__02Etrace_data = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.trace_data;
    vlSelf->picorv32_axi__02Eeoi = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.eoi;
    vlSelf->picorv32_axi__02Epcpi_valid = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.__PVT__irq_active;
    vlSelf->picorv32_axi__02Epcpi_insn = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.pcpi_insn;
    vlSelf->mem_axi_wdata = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_wdata;
    vlSelf->mem_axi_awaddr = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_addr;
    if (vlSelf->resetn) {
        if (((IData)(vlSelf->mem_axi_awready) & (IData)(vlSelf->mem_axi_awvalid))) {
            vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_awvalid = 1U;
        }
        if (((IData)(vlSelf->mem_axi_arready) & (IData)(vlSelf->mem_axi_arvalid))) {
            vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_arvalid = 1U;
        }
        if (((IData)(vlSelf->mem_axi_wready) & (IData)(vlSelf->mem_axi_wvalid))) {
            vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_wvalid = 1U;
        }
        if ((1U & ((IData)(vlSelf->picorv32_axi__DOT__axi_adapter__DOT__xfer_done) 
                   | (~ (IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_valid))))) {
            vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_awvalid = 0U;
            vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_arvalid = 0U;
            vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_wvalid = 0U;
        }
        vlSelf->picorv32_axi__DOT__axi_adapter__DOT__xfer_done 
            = ((IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_valid) 
               & (IData)(vlSelf->picorv32_axi__DOT__mem_ready));
    } else {
        vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_awvalid = 0U;
    }
    vlSelf->mem_axi_araddr = vlSelf->mem_axi_awaddr;
}

VL_INLINE_OPT void Vpicorv32___024root___nba_sequent__TOP__2(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___nba_sequent__TOP__2\n"); );
    // Body
    vlSelf->mem_axi_arprot = ((IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_instr)
                               ? 4U : 0U);
    vlSelf->mem_axi_wstrb = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_wstrb;
    vlSelf->mem_axi_rready = ((~ (IData)((0U != (IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_wstrb)))) 
                              & (IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_valid));
    vlSelf->mem_axi_bready = ((IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_valid) 
                              & (0U != (IData)(vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.mem_wstrb)));
    vlSelf->picorv32_axi__02Etrap = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.trap;
    vlSelf->picorv32_axi__02Epcpi_rs2 = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.__PVT__reg_op2;
    vlSelf->picorv32_axi__02Epcpi_rs1 = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.__PVT__reg_op1;
    vlSelf->picorv32_axi__02Etrace_valid = vlSymsp->TOP__picorv32_axi__DOT__picorv32_core.__PVT__do_waitirq;
    vlSelf->mem_axi_arvalid = ((~ (IData)(vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_arvalid)) 
                               & (IData)(vlSelf->mem_axi_rready));
    vlSelf->mem_axi_awvalid = ((~ (IData)(vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_awvalid)) 
                               & (IData)(vlSelf->mem_axi_bready));
    vlSelf->mem_axi_wvalid = ((~ (IData)(vlSelf->picorv32_axi__DOT__axi_adapter__DOT__ack_wvalid)) 
                              & (IData)(vlSelf->mem_axi_bready));
}

VL_INLINE_OPT void Vpicorv32___024root___nba_sequent__TOP__3(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___nba_sequent__TOP__3\n"); );
    // Init
    CData/*1:0*/ __Vdly__picorv32_wb__DOT__state;
    __Vdly__picorv32_wb__DOT__state = 0;
    // Body
    __Vdly__picorv32_wb__DOT__state = vlSelf->picorv32_wb__DOT__state;
    vlSelf->__Vdly__picorv32_wb__DOT__mem_rdata = vlSelf->picorv32_wb__DOT__mem_rdata;
    vlSelf->__Vdly__picorv32_wb__DOT__mem_ready = vlSelf->picorv32_wb__DOT__mem_ready;
    vlSelf->picorv32_wb__02Etrace_data = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.trace_data;
    vlSelf->picorv32_wb__02Eeoi = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.eoi;
    vlSelf->picorv32_wb__02Epcpi_valid = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.__PVT__irq_active;
    vlSelf->picorv32_wb__02Epcpi_insn = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.pcpi_insn;
    if (vlSelf->wb_rst_i) {
        vlSelf->wbm_adr_o = 0U;
        vlSelf->wbm_dat_o = 0U;
        vlSelf->wbm_we_o = 0U;
        vlSelf->wbm_sel_o = 0U;
        vlSelf->wbm_stb_o = 0U;
        vlSelf->wbm_cyc_o = 0U;
        __Vdly__picorv32_wb__DOT__state = 0U;
    } else if ((0U == (IData)(vlSelf->picorv32_wb__DOT__state))) {
        if (vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.mem_valid) {
            vlSelf->wbm_adr_o = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.mem_addr;
            vlSelf->wbm_dat_o = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.mem_wdata;
            vlSelf->wbm_we_o = (IData)((0U != (IData)(vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.mem_wstrb)));
            vlSelf->wbm_sel_o = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.mem_wstrb;
            vlSelf->wbm_stb_o = 1U;
            vlSelf->wbm_cyc_o = 1U;
            __Vdly__picorv32_wb__DOT__state = 1U;
        } else {
            vlSelf->__Vdly__picorv32_wb__DOT__mem_ready = 0U;
            vlSelf->wbm_stb_o = 0U;
            vlSelf->wbm_cyc_o = 0U;
            vlSelf->wbm_we_o = 0U;
        }
    } else if ((1U == (IData)(vlSelf->picorv32_wb__DOT__state))) {
        if (vlSelf->wbm_ack_i) {
            vlSelf->__Vdly__picorv32_wb__DOT__mem_rdata 
                = vlSelf->wbm_dat_i;
            vlSelf->__Vdly__picorv32_wb__DOT__mem_ready = 1U;
            __Vdly__picorv32_wb__DOT__state = 2U;
            vlSelf->wbm_stb_o = 0U;
            vlSelf->wbm_cyc_o = 0U;
            vlSelf->wbm_we_o = 0U;
        }
    } else if ((2U == (IData)(vlSelf->picorv32_wb__DOT__state))) {
        vlSelf->__Vdly__picorv32_wb__DOT__mem_ready = 0U;
        __Vdly__picorv32_wb__DOT__state = 0U;
    } else {
        __Vdly__picorv32_wb__DOT__state = 0U;
    }
    vlSelf->picorv32_wb__DOT__state = __Vdly__picorv32_wb__DOT__state;
}

VL_INLINE_OPT void Vpicorv32___024root___nba_sequent__TOP__4(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___nba_sequent__TOP__4\n"); );
    // Body
    vlSelf->picorv32_wb__DOT__mem_ready = vlSelf->__Vdly__picorv32_wb__DOT__mem_ready;
    vlSelf->mem_instr = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.mem_instr;
    vlSelf->picorv32_wb__02Etrap = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.trap;
    vlSelf->picorv32_wb__02Epcpi_rs2 = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.__PVT__reg_op2;
    vlSelf->picorv32_wb__02Epcpi_rs1 = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.__PVT__reg_op1;
    vlSelf->picorv32_wb__02Etrace_valid = vlSymsp->TOP__picorv32_wb__DOT__picorv32_core.__PVT__do_waitirq;
    vlSelf->picorv32_wb__DOT__mem_rdata = vlSelf->__Vdly__picorv32_wb__DOT__mem_rdata;
}

void Vpicorv32___024root___nba_sequent__TOP__0(Vpicorv32___024root* vlSelf);
void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf);
void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__1(Vpicorv32_picorv32__pi1* vlSelf);
void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf);
void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__1(Vpicorv32_picorv32__pi1* vlSelf);
void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__2(Vpicorv32_picorv32__pi1* vlSelf);
void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__2(Vpicorv32_picorv32__pi1* vlSelf);

void Vpicorv32___024root___eval_nba(Vpicorv32___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vpicorv32___024root___eval_nba\n"); );
    // Body
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        Vpicorv32___024root___nba_sequent__TOP__0(vlSelf);
    }
    if ((2ULL & vlSelf->__VnbaTriggered.word(0U))) {
        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__0((&vlSymsp->TOP__picorv32_axi__DOT__picorv32_core));
        Vpicorv32___024root___nba_sequent__TOP__1(vlSelf);
        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__1((&vlSymsp->TOP__picorv32_axi__DOT__picorv32_core));
        Vpicorv32___024root___nba_sequent__TOP__2(vlSelf);
    }
    if ((4ULL & vlSelf->__VnbaTriggered.word(0U))) {
        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__0((&vlSymsp->TOP__picorv32_wb__DOT__picorv32_core));
        Vpicorv32___024root___nba_sequent__TOP__3(vlSelf);
        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__1((&vlSymsp->TOP__picorv32_wb__DOT__picorv32_core));
        Vpicorv32___024root___nba_sequent__TOP__4(vlSelf);
        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__2((&vlSymsp->TOP__picorv32_wb__DOT__picorv32_core));
    }
    if ((2ULL & vlSelf->__VnbaTriggered.word(0U))) {
        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__2((&vlSymsp->TOP__picorv32_axi__DOT__picorv32_core));
    }
}
