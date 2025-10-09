// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vpicorv32.h for the primary calling header

#include "Vpicorv32__pch.h"
#include "Vpicorv32__Syms.h"
#include "Vpicorv32_picorv32__pi1.h"

VL_INLINE_OPT void Vpicorv32_picorv32__pi1___ico_sequent__TOP__picorv32_axi__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___ico_sequent__TOP__picorv32_axi__DOT__picorv32_core__0\n"); );
    // Body
    if ((0U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_rdata_word = vlSymsp->TOP.mem_axi_rdata;
    } else if ((1U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        if ((2U & vlSelf->__PVT__reg_op1)) {
            if ((2U & vlSelf->__PVT__reg_op1)) {
                vlSelf->__PVT__mem_rdata_word = (vlSymsp->TOP.mem_axi_rdata 
                                                 >> 0x10U);
            }
        } else {
            vlSelf->__PVT__mem_rdata_word = (0xffffU 
                                             & vlSymsp->TOP.mem_axi_rdata);
        }
    } else if ((2U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_rdata_word = ((2U & vlSelf->__PVT__reg_op1)
                                          ? ((1U & vlSelf->__PVT__reg_op1)
                                              ? (vlSymsp->TOP.mem_axi_rdata 
                                                 >> 0x18U)
                                              : (0xffU 
                                                 & (vlSymsp->TOP.mem_axi_rdata 
                                                    >> 0x10U)))
                                          : ((1U & vlSelf->__PVT__reg_op1)
                                              ? (0xffU 
                                                 & (vlSymsp->TOP.mem_axi_rdata 
                                                    >> 8U))
                                              : (0xffU 
                                                 & vlSymsp->TOP.mem_axi_rdata)));
    }
    vlSelf->__PVT__mem_la_write = ((IData)(vlSymsp->TOP.resetn) 
                                   & ((~ (IData)((0U 
                                                  != (IData)(vlSelf->__PVT__mem_state)))) 
                                      & (IData)(vlSelf->__PVT__mem_do_wdata)));
    vlSelf->__PVT__mem_la_read = ((IData)(vlSymsp->TOP.resetn) 
                                  & ((~ (IData)((0U 
                                                 != (IData)(vlSelf->__PVT__mem_state)))) 
                                     & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                        | ((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                           | (IData)(vlSelf->__PVT__mem_do_rdata)))));
    vlSelf->__PVT__mem_xfer = ((IData)(vlSymsp->TOP.picorv32_axi__DOT__mem_ready) 
                               & (IData)(vlSelf->mem_valid));
    vlSelf->__PVT__mem_rdata_latched = ((IData)(vlSelf->__PVT__mem_xfer)
                                         ? vlSymsp->TOP.mem_axi_rdata
                                         : vlSelf->__PVT__mem_rdata_q);
    vlSelf->__PVT__mem_done = ((IData)(vlSymsp->TOP.resetn) 
                               & (((IData)(vlSelf->__PVT__mem_xfer) 
                                   & ((0U != (IData)(vlSelf->__PVT__mem_state)) 
                                      & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                         | ((IData)(vlSelf->__PVT__mem_do_rdata) 
                                            | (IData)(vlSelf->__PVT__mem_do_wdata))))) 
                                  | ((3U == (IData)(vlSelf->__PVT__mem_state)) 
                                     & (IData)(vlSelf->__PVT__mem_do_rinst))));
}

VL_INLINE_OPT void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__0\n"); );
    // Init
    CData/*4:0*/ __Vdlyvdim0__cpuregs__v0;
    __Vdlyvdim0__cpuregs__v0 = 0;
    IData/*31:0*/ __Vdlyvval__cpuregs__v0;
    __Vdlyvval__cpuregs__v0 = 0;
    CData/*0:0*/ __Vdlyvset__cpuregs__v0;
    __Vdlyvset__cpuregs__v0 = 0;
    CData/*4:0*/ __Vdly__reg_sh;
    __Vdly__reg_sh = 0;
    IData/*31:0*/ __Vdly__reg_out;
    __Vdly__reg_out = 0;
    QData/*63:0*/ __Vdly__count_cycle;
    __Vdly__count_cycle = 0;
    CData/*0:0*/ __Vdly__decoder_trigger;
    __Vdly__decoder_trigger = 0;
    CData/*0:0*/ __Vdly__decoder_pseudo_trigger;
    __Vdly__decoder_pseudo_trigger = 0;
    // Body
    vlSelf->__Vdly__count_instr = vlSelf->__PVT__count_instr;
    vlSelf->__Vdly__latched_is_lb = vlSelf->__PVT__latched_is_lb;
    vlSelf->__Vdly__latched_is_lh = vlSelf->__PVT__latched_is_lh;
    vlSelf->__Vdly__latched_is_lu = vlSelf->__PVT__latched_is_lu;
    __Vdly__decoder_pseudo_trigger = vlSelf->__PVT__decoder_pseudo_trigger;
    __Vdly__count_cycle = vlSelf->__PVT__count_cycle;
    __Vdly__reg_sh = vlSelf->__PVT__reg_sh;
    __Vdly__decoder_trigger = vlSelf->__PVT__decoder_trigger;
    vlSelf->__Vdly__mem_do_prefetch = vlSelf->__PVT__mem_do_prefetch;
    vlSelf->__Vdly__reg_pc = vlSelf->__PVT__reg_pc;
    vlSelf->__Vdly__mem_wordsize = vlSelf->__PVT__mem_wordsize;
    vlSelf->__Vdly__mem_do_rinst = vlSelf->__PVT__mem_do_rinst;
    __Vdly__reg_out = vlSelf->__PVT__reg_out;
    vlSelf->__Vdly__cpu_state = vlSelf->__PVT__cpu_state;
    __Vdlyvset__cpuregs__v0 = 0U;
    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__reg_op1;
    vlSelf->__Vdly__mem_state = vlSelf->__PVT__mem_state;
    if ((((IData)(vlSymsp->TOP.resetn) & (IData)(vlSelf->__PVT__cpuregs_write)) 
         & (0U != (IData)(vlSelf->__PVT__latched_rd)))) {
        __Vdlyvval__cpuregs__v0 = vlSelf->__PVT__cpuregs_wrdata;
        __Vdlyvset__cpuregs__v0 = 1U;
        __Vdlyvdim0__cpuregs__v0 = vlSelf->__PVT__latched_rd;
    }
    vlSelf->trace_data = 0ULL;
    if (((IData)(vlSelf->__PVT__decoder_trigger) & 
         (~ (IData)(vlSelf->__PVT__decoder_pseudo_trigger)))) {
        vlSelf->pcpi_insn = 0U;
        vlSelf->__PVT__instr_setq = 0U;
        vlSelf->__PVT__instr_getq = 0U;
        vlSelf->__PVT__instr_maskirq = 0U;
        vlSelf->__PVT__instr_timer = 0U;
        vlSelf->__PVT__instr_fence = ((0xfU == (0x7fU 
                                                & vlSelf->__PVT__mem_rdata_q)) 
                                      & (~ (IData)(
                                                   (0U 
                                                    != 
                                                    (7U 
                                                     & (vlSelf->__PVT__mem_rdata_q 
                                                        >> 0xcU))))));
    }
    if ((1U & (~ (IData)(vlSymsp->TOP.resetn)))) {
        vlSelf->eoi = 0U;
        vlSelf->__PVT__irq_active = 0U;
        vlSelf->__PVT__instr_fence = 0U;
    }
    if (((IData)(vlSelf->__PVT__mem_do_rinst) & (IData)(vlSelf->__PVT__mem_done))) {
        vlSelf->__PVT__decoded_rs1 = (0x1fU & (vlSelf->__PVT__mem_rdata_latched 
                                               >> 0xfU));
        vlSelf->__PVT__instr_waitirq = 0U;
    }
    if ((1U & (~ ((~ (IData)(vlSymsp->TOP.resetn)) 
                  | (IData)(vlSelf->trap))))) {
        if (vlSelf->__PVT__mem_la_write) {
            vlSelf->mem_wdata = vlSelf->__PVT__mem_la_wdata;
        }
        if (((IData)(vlSelf->__PVT__mem_la_read) | (IData)(vlSelf->__PVT__mem_la_write))) {
            vlSelf->mem_addr = (((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                 | (IData)(vlSelf->__PVT__mem_do_rinst))
                                 ? (0xfffffffcU & vlSelf->__PVT__next_pc)
                                 : (0xfffffffcU & vlSelf->__PVT__reg_op1));
        }
    }
    if (__Vdlyvset__cpuregs__v0) {
        vlSelf->__PVT__cpuregs[__Vdlyvdim0__cpuregs__v0] 
            = __Vdlyvval__cpuregs__v0;
    }
}

VL_INLINE_OPT void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__1(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__1\n"); );
    // Init
    CData/*0:0*/ __PVT__set_mem_do_rinst;
    __PVT__set_mem_do_rinst = 0;
    CData/*0:0*/ __PVT__set_mem_do_rdata;
    __PVT__set_mem_do_rdata = 0;
    CData/*0:0*/ __PVT__set_mem_do_wdata;
    __PVT__set_mem_do_wdata = 0;
    CData/*0:0*/ __PVT__alu_eq;
    __PVT__alu_eq = 0;
    CData/*0:0*/ __PVT__alu_ltu;
    __PVT__alu_ltu = 0;
    CData/*0:0*/ __PVT__alu_lts;
    __PVT__alu_lts = 0;
    CData/*4:0*/ __Vdly__reg_sh;
    __Vdly__reg_sh = 0;
    IData/*31:0*/ __Vdly__reg_out;
    __Vdly__reg_out = 0;
    QData/*63:0*/ __Vdly__count_cycle;
    __Vdly__count_cycle = 0;
    CData/*0:0*/ __Vdly__decoder_trigger;
    __Vdly__decoder_trigger = 0;
    CData/*0:0*/ __Vdly__decoder_pseudo_trigger;
    __Vdly__decoder_pseudo_trigger = 0;
    // Body
    if ((1U & ((~ (IData)(vlSymsp->TOP.resetn)) | (IData)(vlSelf->trap)))) {
        if ((1U & (~ (IData)(vlSymsp->TOP.resetn)))) {
            vlSelf->__Vdly__mem_state = 0U;
        }
        if ((1U & ((~ (IData)(vlSymsp->TOP.resetn)) 
                   | (IData)(vlSymsp->TOP.picorv32_axi__DOT__mem_ready)))) {
            vlSelf->mem_valid = 0U;
        }
    } else {
        if (((IData)(vlSelf->__PVT__mem_la_read) | (IData)(vlSelf->__PVT__mem_la_write))) {
            vlSelf->mem_wstrb = ((IData)(vlSelf->__PVT__mem_la_wstrb) 
                                 & (- (IData)((IData)(vlSelf->__PVT__mem_la_write))));
        }
        if ((0U == (IData)(vlSelf->__PVT__mem_state))) {
            if ((((IData)(vlSelf->__PVT__mem_do_prefetch) 
                  | (IData)(vlSelf->__PVT__mem_do_rinst)) 
                 | (IData)(vlSelf->__PVT__mem_do_rdata))) {
                vlSelf->mem_valid = 1U;
                vlSelf->mem_instr = ((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                     | (IData)(vlSelf->__PVT__mem_do_rinst));
                vlSelf->mem_wstrb = 0U;
                vlSelf->__Vdly__mem_state = 1U;
            }
            if (vlSelf->__PVT__mem_do_wdata) {
                vlSelf->mem_valid = 1U;
                vlSelf->mem_instr = 0U;
                vlSelf->__Vdly__mem_state = 2U;
            }
        } else if ((1U == (IData)(vlSelf->__PVT__mem_state))) {
            if (vlSelf->__PVT__mem_xfer) {
                vlSelf->mem_valid = 0U;
                vlSelf->__Vdly__mem_state = (((IData)(vlSelf->__PVT__mem_do_rinst) 
                                              | (IData)(vlSelf->__PVT__mem_do_rdata))
                                              ? 0U : 3U);
            }
        } else if ((2U == (IData)(vlSelf->__PVT__mem_state))) {
            if (vlSelf->__PVT__mem_xfer) {
                vlSelf->mem_valid = 0U;
                vlSelf->__Vdly__mem_state = 0U;
            }
        } else if ((3U == (IData)(vlSelf->__PVT__mem_state))) {
            if (vlSelf->__PVT__mem_do_rinst) {
                vlSelf->__Vdly__mem_state = 0U;
            }
        }
    }
    vlSelf->__PVT__mem_state = vlSelf->__Vdly__mem_state;
    vlSelf->trap = 0U;
    __Vdly__reg_sh = 0U;
    __Vdly__reg_out = 0U;
    __PVT__set_mem_do_rinst = 0U;
    __PVT__set_mem_do_rdata = 0U;
    __PVT__set_mem_do_wdata = 0U;
    __Vdly__decoder_trigger = ((IData)(vlSelf->__PVT__mem_do_rinst) 
                               & (IData)(vlSelf->__PVT__mem_done));
    __Vdly__decoder_pseudo_trigger = 0U;
    if (vlSymsp->TOP.resetn) {
        __Vdly__count_cycle = (1ULL + vlSelf->__PVT__count_cycle);
        if (((((((((0x80U == (IData)(vlSelf->__PVT__cpu_state)) 
                   | (0x40U == (IData)(vlSelf->__PVT__cpu_state))) 
                  | (0x20U == (IData)(vlSelf->__PVT__cpu_state))) 
                 | (0x10U == (IData)(vlSelf->__PVT__cpu_state))) 
                | (8U == (IData)(vlSelf->__PVT__cpu_state))) 
               | (4U == (IData)(vlSelf->__PVT__cpu_state))) 
              | (2U == (IData)(vlSelf->__PVT__cpu_state))) 
             | (1U == (IData)(vlSelf->__PVT__cpu_state)))) {
            if ((0x80U == (IData)(vlSelf->__PVT__cpu_state))) {
                vlSelf->trap = 1U;
            } else if ((0x40U == (IData)(vlSelf->__PVT__cpu_state))) {
                vlSelf->__Vdly__mem_do_rinst = (1U 
                                                & ((~ (IData)(vlSelf->__PVT__decoder_trigger)) 
                                                   & (~ (IData)(vlSelf->__PVT__do_waitirq))));
                vlSelf->__Vdly__mem_wordsize = 0U;
                vlSelf->__Vdly__latched_is_lu = 0U;
                vlSelf->__Vdly__latched_is_lh = 0U;
                vlSelf->__Vdly__latched_is_lb = 0U;
                vlSelf->__PVT__latched_rd = vlSelf->__PVT__decoded_rd;
                vlSelf->__PVT__latched_compr = vlSelf->__PVT__compressed_instr;
                vlSelf->__PVT__current_pc = vlSelf->__PVT__reg_next_pc;
                if (vlSelf->__PVT__latched_branch) {
                    vlSelf->__PVT__current_pc = ((IData)(vlSelf->__PVT__latched_store)
                                                  ? 
                                                 (0xfffffffeU 
                                                  & ((IData)(vlSelf->__PVT__latched_stalu)
                                                      ? vlSelf->__PVT__alu_out_q
                                                      : vlSelf->__PVT__reg_out))
                                                  : vlSelf->__PVT__reg_next_pc);
                }
                vlSelf->__Vdly__reg_pc = vlSelf->__PVT__current_pc;
                vlSelf->__PVT__reg_next_pc = vlSelf->__PVT__current_pc;
                vlSelf->__PVT__latched_store = 0U;
                vlSelf->__PVT__latched_stalu = 0U;
                vlSelf->__PVT__latched_branch = 0U;
                if (vlSelf->__PVT__decoder_trigger) {
                    vlSelf->__Vdly__count_instr = (1ULL 
                                                   + vlSelf->__PVT__count_instr);
                    vlSelf->__PVT__reg_next_pc = (vlSelf->__PVT__current_pc 
                                                  + 
                                                  ((IData)(vlSelf->__PVT__compressed_instr)
                                                    ? 2U
                                                    : 4U));
                    if (vlSelf->__PVT__instr_jal) {
                        vlSelf->__Vdly__mem_do_rinst = 1U;
                        vlSelf->__PVT__reg_next_pc 
                            = (vlSelf->__PVT__current_pc 
                               + vlSelf->__PVT__decoded_imm_j);
                        vlSelf->__PVT__latched_branch = 1U;
                    } else {
                        vlSelf->__Vdly__mem_do_rinst = 0U;
                        vlSelf->__Vdly__mem_do_prefetch 
                            = (1U & ((~ (IData)(vlSelf->__PVT__instr_jalr)) 
                                     & (~ (IData)(vlSelf->__PVT__compressed_instr))));
                        vlSelf->__Vdly__cpu_state = 0x20U;
                    }
                }
            } else if ((0x20U == (IData)(vlSelf->__PVT__cpu_state))) {
                vlSelf->__Vdly__reg_op1 = 0U;
                vlSelf->__PVT__reg_op2 = 0U;
                if ((((IData)(vlSelf->__PVT__instr_trap) 
                      | (IData)(vlSelf->__PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh)) 
                     | (IData)(vlSelf->__PVT__is_lui_auipc_jal))) {
                    if (vlSelf->__PVT__instr_trap) {
                        vlSelf->__Vdly__cpu_state = 0x80U;
                    } else if (vlSelf->__PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh) {
                        if (vlSelf->__PVT__instr_rdcycle) {
                            __Vdly__reg_out = (IData)(vlSelf->__PVT__count_cycle);
                        } else if (vlSelf->__PVT__instr_rdcycleh) {
                            __Vdly__reg_out = (IData)(
                                                      (vlSelf->__PVT__count_cycle 
                                                       >> 0x20U));
                        } else if (vlSelf->__PVT__instr_rdinstr) {
                            __Vdly__reg_out = (IData)(vlSelf->__PVT__count_instr);
                        } else if (vlSelf->__PVT__instr_rdinstrh) {
                            __Vdly__reg_out = (IData)(
                                                      (vlSelf->__PVT__count_instr 
                                                       >> 0x20U));
                        }
                        vlSelf->__PVT__latched_store = 1U;
                        vlSelf->__Vdly__cpu_state = 0x40U;
                    } else if (vlSelf->__PVT__is_lui_auipc_jal) {
                        vlSelf->__Vdly__reg_op1 = ((IData)(vlSelf->__PVT__instr_lui)
                                                    ? 0U
                                                    : vlSelf->__PVT__reg_pc);
                        vlSelf->__PVT__reg_op2 = vlSelf->__PVT__decoded_imm;
                        vlSelf->__Vdly__mem_do_rinst 
                            = vlSelf->__PVT__mem_do_prefetch;
                        vlSelf->__Vdly__cpu_state = 8U;
                    } else {
                        vlSelf->__PVT__latched_store = 1U;
                        __Vdly__reg_out = vlSelf->__PVT__timer;
                        vlSelf->__Vdly__cpu_state = 0x40U;
                        vlSelf->__PVT__timer = vlSelf->__PVT__cpuregs_rs1;
                    }
                } else if (((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                            & (~ (IData)(vlSelf->__PVT__instr_trap)))) {
                    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__cpuregs_rs1;
                    vlSelf->__Vdly__cpu_state = 1U;
                    vlSelf->__Vdly__mem_do_rinst = 1U;
                } else if (vlSelf->__PVT__is_slli_srli_srai) {
                    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__cpuregs_rs1;
                    __Vdly__reg_sh = vlSelf->__PVT__decoded_rs2;
                    vlSelf->__Vdly__cpu_state = 4U;
                } else if (vlSelf->__PVT__is_jalr_addi_slti_sltiu_xori_ori_andi) {
                    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__cpuregs_rs1;
                    vlSelf->__PVT__reg_op2 = vlSelf->__PVT__decoded_imm;
                    vlSelf->__Vdly__mem_do_rinst = vlSelf->__PVT__mem_do_prefetch;
                    vlSelf->__Vdly__cpu_state = 8U;
                } else {
                    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__cpuregs_rs1;
                    __Vdly__reg_sh = (0x1fU & vlSelf->__PVT__cpuregs_rs2);
                    vlSelf->__PVT__reg_op2 = vlSelf->__PVT__cpuregs_rs2;
                    if (vlSelf->__PVT__is_sb_sh_sw) {
                        vlSelf->__Vdly__cpu_state = 2U;
                        vlSelf->__Vdly__mem_do_rinst = 1U;
                    } else if (vlSelf->__PVT__is_sll_srl_sra) {
                        vlSelf->__Vdly__cpu_state = 4U;
                    } else {
                        vlSelf->__Vdly__mem_do_rinst 
                            = vlSelf->__PVT__mem_do_prefetch;
                        vlSelf->__Vdly__cpu_state = 8U;
                    }
                }
            } else if ((0x10U == (IData)(vlSelf->__PVT__cpu_state))) {
                __Vdly__reg_sh = (0x1fU & vlSelf->__PVT__cpuregs_rs2);
                vlSelf->__PVT__reg_op2 = vlSelf->__PVT__cpuregs_rs2;
                if (vlSelf->__PVT__is_sb_sh_sw) {
                    vlSelf->__Vdly__cpu_state = 2U;
                    vlSelf->__Vdly__mem_do_rinst = 1U;
                } else if (vlSelf->__PVT__is_sll_srl_sra) {
                    vlSelf->__Vdly__cpu_state = 4U;
                } else {
                    vlSelf->__Vdly__mem_do_rinst = vlSelf->__PVT__mem_do_prefetch;
                    vlSelf->__Vdly__cpu_state = 8U;
                }
            } else if ((8U == (IData)(vlSelf->__PVT__cpu_state))) {
                __Vdly__reg_out = (vlSelf->__PVT__reg_pc 
                                   + vlSelf->__PVT__decoded_imm);
                if (vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) {
                    vlSelf->__PVT__latched_rd = 0U;
                    vlSelf->__PVT__latched_store = vlSelf->__PVT__alu_out_0;
                    vlSelf->__PVT__latched_branch = vlSelf->__PVT__alu_out_0;
                    if (vlSelf->__PVT__mem_done) {
                        vlSelf->__Vdly__cpu_state = 0x40U;
                    }
                    if (vlSelf->__PVT__alu_out_0) {
                        __PVT__set_mem_do_rinst = 1U;
                        __Vdly__decoder_trigger = 0U;
                    }
                } else {
                    vlSelf->__PVT__latched_branch = vlSelf->__PVT__instr_jalr;
                    vlSelf->__PVT__latched_store = 1U;
                    vlSelf->__PVT__latched_stalu = 1U;
                    vlSelf->__Vdly__cpu_state = 0x40U;
                }
            } else if ((4U == (IData)(vlSelf->__PVT__cpu_state))) {
                vlSelf->__PVT__latched_store = 1U;
                if ((0U == (IData)(vlSelf->__PVT__reg_sh))) {
                    __Vdly__reg_out = vlSelf->__PVT__reg_op1;
                    vlSelf->__Vdly__mem_do_rinst = vlSelf->__PVT__mem_do_prefetch;
                    vlSelf->__Vdly__cpu_state = 0x40U;
                } else if ((4U <= (IData)(vlSelf->__PVT__reg_sh))) {
                    if (((IData)(vlSelf->__PVT__instr_slli) 
                         | (IData)(vlSelf->__PVT__instr_sll))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTL_III(32,32,32, vlSelf->__PVT__reg_op1, 4U);
                    } else if (((IData)(vlSelf->__PVT__instr_srli) 
                                | (IData)(vlSelf->__PVT__instr_srl))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTR_III(32,32,32, vlSelf->__PVT__reg_op1, 4U);
                    } else if (((IData)(vlSelf->__PVT__instr_srai) 
                                | (IData)(vlSelf->__PVT__instr_sra))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTRS_III(32,32,32, vlSelf->__PVT__reg_op1, 4U);
                    }
                    __Vdly__reg_sh = (0x1fU & ((IData)(vlSelf->__PVT__reg_sh) 
                                               - (IData)(4U)));
                } else {
                    if (((IData)(vlSelf->__PVT__instr_slli) 
                         | (IData)(vlSelf->__PVT__instr_sll))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTL_III(32,32,32, vlSelf->__PVT__reg_op1, 1U);
                    } else if (((IData)(vlSelf->__PVT__instr_srli) 
                                | (IData)(vlSelf->__PVT__instr_srl))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTR_III(32,32,32, vlSelf->__PVT__reg_op1, 1U);
                    } else if (((IData)(vlSelf->__PVT__instr_srai) 
                                | (IData)(vlSelf->__PVT__instr_sra))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTRS_III(32,32,32, vlSelf->__PVT__reg_op1, 1U);
                    }
                    __Vdly__reg_sh = (0x1fU & ((IData)(vlSelf->__PVT__reg_sh) 
                                               - (IData)(1U)));
                }
            } else if ((2U == (IData)(vlSelf->__PVT__cpu_state))) {
                if ((1U & ((~ (IData)(vlSelf->__PVT__mem_do_prefetch)) 
                           | (IData)(vlSelf->__PVT__mem_done)))) {
                    if ((1U & (~ (IData)(vlSelf->__PVT__mem_do_wdata)))) {
                        vlSelf->__Vdly__reg_op1 = (vlSelf->__PVT__reg_op1 
                                                   + vlSelf->__PVT__decoded_imm);
                        __PVT__set_mem_do_wdata = 1U;
                        if (vlSelf->__PVT__instr_sb) {
                            vlSelf->__Vdly__mem_wordsize = 2U;
                        } else if (vlSelf->__PVT__instr_sh) {
                            vlSelf->__Vdly__mem_wordsize = 1U;
                        } else if (vlSelf->__PVT__instr_sw) {
                            vlSelf->__Vdly__mem_wordsize = 0U;
                        }
                    }
                    if (((~ (IData)(vlSelf->__PVT__mem_do_prefetch)) 
                         & (IData)(vlSelf->__PVT__mem_done))) {
                        vlSelf->__Vdly__cpu_state = 0x40U;
                        __Vdly__decoder_trigger = 1U;
                        __Vdly__decoder_pseudo_trigger = 1U;
                    }
                }
            } else {
                vlSelf->__PVT__latched_store = 1U;
                if ((1U & ((~ (IData)(vlSelf->__PVT__mem_do_prefetch)) 
                           | (IData)(vlSelf->__PVT__mem_done)))) {
                    if (((~ (IData)(vlSelf->__PVT__mem_do_prefetch)) 
                         & (IData)(vlSelf->__PVT__mem_done))) {
                        if (vlSelf->__PVT__latched_is_lu) {
                            __Vdly__reg_out = vlSelf->__PVT__mem_rdata_word;
                        } else if (vlSelf->__PVT__latched_is_lh) {
                            __Vdly__reg_out = VL_EXTENDS_II(32,16, 
                                                            (0xffffU 
                                                             & vlSelf->__PVT__mem_rdata_word));
                        } else if (vlSelf->__PVT__latched_is_lb) {
                            __Vdly__reg_out = VL_EXTENDS_II(32,8, 
                                                            (0xffU 
                                                             & vlSelf->__PVT__mem_rdata_word));
                        }
                        __Vdly__decoder_trigger = 1U;
                        __Vdly__decoder_pseudo_trigger = 1U;
                        vlSelf->__Vdly__cpu_state = 0x40U;
                    }
                    if ((1U & (~ (IData)(vlSelf->__PVT__mem_do_rdata)))) {
                        vlSelf->__Vdly__reg_op1 = (vlSelf->__PVT__reg_op1 
                                                   + vlSelf->__PVT__decoded_imm);
                        __PVT__set_mem_do_rdata = 1U;
                        if (((IData)(vlSelf->__PVT__instr_lb) 
                             | (IData)(vlSelf->__PVT__instr_lbu))) {
                            vlSelf->__Vdly__mem_wordsize = 2U;
                        } else if (((IData)(vlSelf->__PVT__instr_lh) 
                                    | (IData)(vlSelf->__PVT__instr_lhu))) {
                            vlSelf->__Vdly__mem_wordsize = 1U;
                        } else if (vlSelf->__PVT__instr_lw) {
                            vlSelf->__Vdly__mem_wordsize = 0U;
                        }
                        vlSelf->__Vdly__latched_is_lu 
                            = vlSelf->__PVT__is_lbu_lhu_lw;
                        vlSelf->__Vdly__latched_is_lh 
                            = vlSelf->__PVT__instr_lh;
                        vlSelf->__Vdly__latched_is_lb 
                            = vlSelf->__PVT__instr_lb;
                    }
                }
            }
        }
    } else {
        __Vdly__count_cycle = 0ULL;
        vlSelf->__Vdly__count_instr = 0ULL;
        vlSelf->__PVT__timer = 0U;
        vlSelf->__Vdly__reg_pc = 0U;
        vlSelf->__PVT__reg_next_pc = 0U;
        vlSelf->__PVT__latched_store = 0U;
        vlSelf->__PVT__latched_stalu = 0U;
        vlSelf->__PVT__latched_branch = 0U;
        vlSelf->__Vdly__latched_is_lu = 0U;
        vlSelf->__Vdly__latched_is_lh = 0U;
        vlSelf->__Vdly__latched_is_lb = 0U;
        vlSelf->__Vdly__cpu_state = 0x40U;
    }
    if (((IData)(vlSymsp->TOP.resetn) & ((IData)(vlSelf->__PVT__mem_do_rdata) 
                                         | (IData)(vlSelf->__PVT__mem_do_wdata)))) {
        if (((0U == (IData)(vlSelf->__PVT__mem_wordsize)) 
             & (0U != (3U & vlSelf->__PVT__reg_op1)))) {
            vlSelf->__Vdly__cpu_state = 0x80U;
        }
        if (((1U == (IData)(vlSelf->__PVT__mem_wordsize)) 
             & vlSelf->__PVT__reg_op1)) {
            vlSelf->__Vdly__cpu_state = 0x80U;
        }
    }
    if ((((IData)(vlSymsp->TOP.resetn) & (IData)(vlSelf->__PVT__mem_do_rinst)) 
         & (0U != (3U & vlSelf->__PVT__reg_pc)))) {
        vlSelf->__Vdly__cpu_state = 0x80U;
    }
    if ((1U & ((~ (IData)(vlSymsp->TOP.resetn)) | (IData)(vlSelf->__PVT__mem_done)))) {
        vlSelf->__Vdly__mem_do_prefetch = 0U;
        vlSelf->__Vdly__mem_do_rinst = 0U;
        vlSelf->__PVT__mem_do_rdata = 0U;
        vlSelf->__PVT__mem_do_wdata = 0U;
    }
    if (__PVT__set_mem_do_rinst) {
        vlSelf->__Vdly__mem_do_rinst = 1U;
    }
    if (__PVT__set_mem_do_rdata) {
        vlSelf->__PVT__mem_do_rdata = 1U;
    }
    if (__PVT__set_mem_do_wdata) {
        vlSelf->__PVT__mem_do_wdata = 1U;
    }
    vlSelf->__PVT__current_pc = 0U;
    vlSelf->__PVT__reg_sh = __Vdly__reg_sh;
    vlSelf->__PVT__count_cycle = __Vdly__count_cycle;
    vlSelf->__PVT__latched_is_lu = vlSelf->__Vdly__latched_is_lu;
    vlSelf->__PVT__latched_is_lh = vlSelf->__Vdly__latched_is_lh;
    vlSelf->__PVT__latched_is_lb = vlSelf->__Vdly__latched_is_lb;
    vlSelf->__PVT__count_instr = vlSelf->__Vdly__count_instr;
    vlSelf->__PVT__cpuregs_rs1 = ((0U != (IData)(vlSelf->__PVT__decoded_rs1))
                                   ? vlSelf->__PVT__cpuregs
                                  [vlSelf->__PVT__decoded_rs1]
                                   : 0U);
    vlSelf->__PVT__mem_do_prefetch = vlSelf->__Vdly__mem_do_prefetch;
    vlSelf->__PVT__reg_pc = vlSelf->__Vdly__reg_pc;
    vlSelf->__PVT__mem_wordsize = vlSelf->__Vdly__mem_wordsize;
    vlSelf->__PVT__reg_out = __Vdly__reg_out;
    vlSelf->__PVT__cpu_state = vlSelf->__Vdly__cpu_state;
    vlSelf->__PVT__reg_op1 = vlSelf->__Vdly__reg_op1;
    vlSelf->__PVT__mem_la_write = ((IData)(vlSymsp->TOP.resetn) 
                                   & ((~ (IData)((0U 
                                                  != (IData)(vlSelf->__PVT__mem_state)))) 
                                      & (IData)(vlSelf->__PVT__mem_do_wdata)));
    vlSelf->__PVT__do_waitirq = 0U;
    vlSelf->__PVT__cpuregs_write = 0U;
    vlSelf->__PVT__next_pc = (((IData)(vlSelf->__PVT__latched_branch) 
                               & (IData)(vlSelf->__PVT__latched_store))
                               ? (0xfffffffeU & vlSelf->__PVT__reg_out)
                               : vlSelf->__PVT__reg_next_pc);
    vlSelf->__PVT__alu_out_q = vlSelf->__PVT__alu_out;
    if ((0U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_la_wdata = vlSelf->__PVT__reg_op2;
        vlSelf->__PVT__mem_la_wstrb = 0xfU;
        vlSelf->__PVT__mem_rdata_word = vlSymsp->TOP.mem_axi_rdata;
    } else if ((1U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_la_wdata = ((vlSelf->__PVT__reg_op2 
                                        << 0x10U) | 
                                       (0xffffU & vlSelf->__PVT__reg_op2));
        if ((2U & vlSelf->__PVT__reg_op1)) {
            vlSelf->__PVT__mem_la_wstrb = 0xcU;
            if ((2U & vlSelf->__PVT__reg_op1)) {
                vlSelf->__PVT__mem_rdata_word = (vlSymsp->TOP.mem_axi_rdata 
                                                 >> 0x10U);
            }
        } else {
            vlSelf->__PVT__mem_la_wstrb = 3U;
            vlSelf->__PVT__mem_rdata_word = (0xffffU 
                                             & vlSymsp->TOP.mem_axi_rdata);
        }
    } else if ((2U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_la_wdata = ((vlSelf->__PVT__reg_op2 
                                        << 0x18U) | 
                                       ((0xff0000U 
                                         & (vlSelf->__PVT__reg_op2 
                                            << 0x10U)) 
                                        | ((0xff00U 
                                            & (vlSelf->__PVT__reg_op2 
                                               << 8U)) 
                                           | (0xffU 
                                              & vlSelf->__PVT__reg_op2))));
        vlSelf->__PVT__mem_la_wstrb = (0xfU & ((IData)(1U) 
                                               << (3U 
                                                   & vlSelf->__PVT__reg_op1)));
        vlSelf->__PVT__mem_rdata_word = ((2U & vlSelf->__PVT__reg_op1)
                                          ? ((1U & vlSelf->__PVT__reg_op1)
                                              ? (vlSymsp->TOP.mem_axi_rdata 
                                                 >> 0x18U)
                                              : (0xffU 
                                                 & (vlSymsp->TOP.mem_axi_rdata 
                                                    >> 0x10U)))
                                          : ((1U & vlSelf->__PVT__reg_op1)
                                              ? (0xffU 
                                                 & (vlSymsp->TOP.mem_axi_rdata 
                                                    >> 8U))
                                              : (0xffU 
                                                 & vlSymsp->TOP.mem_axi_rdata)));
    }
    __PVT__alu_eq = (vlSelf->__PVT__reg_op1 == vlSelf->__PVT__reg_op2);
    __PVT__alu_lts = VL_LTS_III(32, vlSelf->__PVT__reg_op1, vlSelf->__PVT__reg_op2);
    __PVT__alu_ltu = (vlSelf->__PVT__reg_op1 < vlSelf->__PVT__reg_op2);
    vlSelf->__PVT__is_lbu_lhu_lw = ((IData)(vlSelf->__PVT__instr_lbu) 
                                    | ((IData)(vlSelf->__PVT__instr_lhu) 
                                       | (IData)(vlSelf->__PVT__instr_lw)));
    vlSelf->__PVT__cpuregs_wrdata = 0U;
    if ((0x40U == (IData)(vlSelf->__PVT__cpu_state))) {
        if (vlSelf->__PVT__latched_branch) {
            vlSelf->__PVT__cpuregs_write = 1U;
            vlSelf->__PVT__cpuregs_wrdata = (vlSelf->__PVT__reg_pc 
                                             + ((IData)(vlSelf->__PVT__latched_compr)
                                                 ? 2U
                                                 : 4U));
        } else if (((IData)(vlSelf->__PVT__latched_store) 
                    & (~ (IData)(vlSelf->__PVT__latched_branch)))) {
            vlSelf->__PVT__cpuregs_write = 1U;
            vlSelf->__PVT__cpuregs_wrdata = ((IData)(vlSelf->__PVT__latched_stalu)
                                              ? vlSelf->__PVT__alu_out_q
                                              : vlSelf->__PVT__reg_out);
        }
    }
    if (((IData)(vlSelf->__PVT__mem_do_rinst) & (IData)(vlSelf->__PVT__mem_done))) {
        vlSelf->__PVT__decoded_rd = (0x1fU & (vlSelf->__PVT__mem_rdata_latched 
                                              >> 7U));
        vlSelf->__PVT__decoded_rs2 = (0x1fU & (vlSelf->__PVT__mem_rdata_latched 
                                               >> 0x14U));
    }
    vlSelf->__PVT__cpuregs_rs2 = ((0U != (IData)(vlSelf->__PVT__decoded_rs2))
                                   ? vlSelf->__PVT__cpuregs
                                  [vlSelf->__PVT__decoded_rs2]
                                   : 0U);
    if (((IData)(vlSelf->__PVT__decoder_trigger) & 
         (~ (IData)(vlSelf->__PVT__decoder_pseudo_trigger)))) {
        vlSelf->__PVT__instr_rdcycle = ((IData)((0xc0002073U 
                                                 == 
                                                 (0xfffff07fU 
                                                  & vlSelf->__PVT__mem_rdata_q))) 
                                        | (IData)((0xc0102073U 
                                                   == 
                                                   (0xfffff07fU 
                                                    & vlSelf->__PVT__mem_rdata_q))));
        vlSelf->__PVT__instr_rdcycleh = ((IData)((0xc8002073U 
                                                  == 
                                                  (0xfffff07fU 
                                                   & vlSelf->__PVT__mem_rdata_q))) 
                                         | (IData)(
                                                   (0xc8102073U 
                                                    == 
                                                    (0xfffff07fU 
                                                     & vlSelf->__PVT__mem_rdata_q))));
        vlSelf->__PVT__instr_rdinstr = (IData)((0xc0202073U 
                                                == 
                                                (0xfffff07fU 
                                                 & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_rdinstrh = (IData)((0xc8202073U 
                                                 == 
                                                 (0xfffff07fU 
                                                  & vlSelf->__PVT__mem_rdata_q)));
    }
    vlSelf->__PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh 
        = ((IData)(vlSelf->__PVT__instr_rdcycle) | 
           ((IData)(vlSelf->__PVT__instr_rdcycleh) 
            | ((IData)(vlSelf->__PVT__instr_rdinstr) 
               | (IData)(vlSelf->__PVT__instr_rdinstrh))));
    vlSelf->__PVT__is_lui_auipc_jal = ((IData)(vlSelf->__PVT__instr_lui) 
                                       | ((IData)(vlSelf->__PVT__instr_auipc) 
                                          | (IData)(vlSelf->__PVT__instr_jal)));
    vlSelf->__PVT__is_lui_auipc_jal_jalr_addi_add_sub 
        = ((IData)(vlSelf->__PVT__instr_lui) | ((IData)(vlSelf->__PVT__instr_auipc) 
                                                | ((IData)(vlSelf->__PVT__instr_jal) 
                                                   | ((IData)(vlSelf->__PVT__instr_jalr) 
                                                      | ((IData)(vlSelf->__PVT__instr_addi) 
                                                         | ((IData)(vlSelf->__PVT__instr_add) 
                                                            | (IData)(vlSelf->__PVT__instr_sub)))))));
    vlSelf->__PVT__is_slti_blt_slt = ((IData)(vlSelf->__PVT__instr_slti) 
                                      | ((IData)(vlSelf->__PVT__instr_blt) 
                                         | (IData)(vlSelf->__PVT__instr_slt)));
    vlSelf->__PVT__is_sltiu_bltu_sltu = ((IData)(vlSelf->__PVT__instr_sltiu) 
                                         | ((IData)(vlSelf->__PVT__instr_bltu) 
                                            | (IData)(vlSelf->__PVT__instr_sltu)));
    vlSelf->__PVT__is_compare = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                 | ((IData)(vlSelf->__PVT__instr_slti) 
                                    | ((IData)(vlSelf->__PVT__instr_slt) 
                                       | ((IData)(vlSelf->__PVT__instr_sltiu) 
                                          | (IData)(vlSelf->__PVT__instr_sltu)))));
    if (((IData)(vlSelf->__PVT__decoder_trigger) & 
         (~ (IData)(vlSelf->__PVT__decoder_pseudo_trigger)))) {
        vlSelf->__PVT__instr_beq = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                    & (0U == (0x7000U 
                                              & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_bne = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                    & (0x1000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_blt = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                    & (0x4000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_bge = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                    & (0x5000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_bltu = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                     & (0x6000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_bgeu = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                     & (0x7000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lb = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                   & (0U == (0x7000U 
                                             & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lh = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                   & (0x1000U == (0x7000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lw = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                   & (0x2000U == (0x7000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lbu = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                    & (0x4000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lhu = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                    & (0x5000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sb = ((IData)(vlSelf->__PVT__is_sb_sh_sw) 
                                   & (0U == (0x7000U 
                                             & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sh = ((IData)(vlSelf->__PVT__is_sb_sh_sw) 
                                   & (0x1000U == (0x7000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sw = ((IData)(vlSelf->__PVT__is_sb_sh_sw) 
                                   & (0x2000U == (0x7000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_addi = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0U == (0x7000U 
                                               & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_slti = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x2000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sltiu = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                      & (0x3000U == 
                                         (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_xori = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x4000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_ori = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                    & (0x6000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_andi = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x7000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_slli = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x1000U == 
                                        (0xfe007000U 
                                         & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_srli = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x5000U == 
                                        (0xfe007000U 
                                         & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_srai = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x40005000U 
                                        == (0xfe007000U 
                                            & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__is_slli_srli_srai = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                            & ((IData)(
                                                       (0x1000U 
                                                        == 
                                                        (0xfe007000U 
                                                         & vlSelf->__PVT__mem_rdata_q))) 
                                               | ((IData)(
                                                          (0x5000U 
                                                           == 
                                                           (0xfe007000U 
                                                            & vlSelf->__PVT__mem_rdata_q))) 
                                                  | (IData)(
                                                            (0x40005000U 
                                                             == 
                                                             (0xfe007000U 
                                                              & vlSelf->__PVT__mem_rdata_q))))));
        vlSelf->__PVT__is_jalr_addi_slti_sltiu_xori_ori_andi 
            = ((IData)(vlSelf->__PVT__instr_jalr) | 
               ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                & ((0U == (7U & (vlSelf->__PVT__mem_rdata_q 
                                 >> 0xcU))) | ((2U 
                                                == 
                                                (7U 
                                                 & (vlSelf->__PVT__mem_rdata_q 
                                                    >> 0xcU))) 
                                               | ((3U 
                                                   == 
                                                   (7U 
                                                    & (vlSelf->__PVT__mem_rdata_q 
                                                       >> 0xcU))) 
                                                  | ((4U 
                                                      == 
                                                      (7U 
                                                       & (vlSelf->__PVT__mem_rdata_q 
                                                          >> 0xcU))) 
                                                     | ((6U 
                                                         == 
                                                         (7U 
                                                          & (vlSelf->__PVT__mem_rdata_q 
                                                             >> 0xcU))) 
                                                        | (7U 
                                                           == 
                                                           (7U 
                                                            & (vlSelf->__PVT__mem_rdata_q 
                                                               >> 0xcU))))))))));
        vlSelf->__PVT__is_lui_auipc_jal_jalr_addi_add_sub = 0U;
        vlSelf->__PVT__is_compare = 0U;
        vlSelf->__PVT__decoded_imm = ((IData)(vlSelf->__PVT__instr_jal)
                                       ? vlSelf->__PVT__decoded_imm_j
                                       : (((IData)(vlSelf->__PVT__instr_lui) 
                                           | (IData)(vlSelf->__PVT__instr_auipc))
                                           ? VL_SHIFTL_III(32,32,32, 
                                                           (vlSelf->__PVT__mem_rdata_q 
                                                            >> 0xcU), 0xcU)
                                           : (((IData)(vlSelf->__PVT__instr_jalr) 
                                               | ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                                  | (IData)(vlSelf->__PVT__is_alu_reg_imm)))
                                               ? VL_EXTENDS_II(32,12, 
                                                               (vlSelf->__PVT__mem_rdata_q 
                                                                >> 0x14U))
                                               : ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu)
                                                   ? 
                                                  VL_EXTENDS_II(32,13, 
                                                                ((0x1000U 
                                                                  & (vlSelf->__PVT__mem_rdata_q 
                                                                     >> 0x13U)) 
                                                                 | ((0x800U 
                                                                     & (vlSelf->__PVT__mem_rdata_q 
                                                                        << 4U)) 
                                                                    | ((0x7e0U 
                                                                        & (vlSelf->__PVT__mem_rdata_q 
                                                                           >> 0x14U)) 
                                                                       | (0x1eU 
                                                                          & (vlSelf->__PVT__mem_rdata_q 
                                                                             >> 7U))))))
                                                   : 
                                                  ((IData)(vlSelf->__PVT__is_sb_sh_sw)
                                                    ? 
                                                   VL_EXTENDS_II(32,12, 
                                                                 ((0xfe0U 
                                                                   & (vlSelf->__PVT__mem_rdata_q 
                                                                      >> 0x14U)) 
                                                                  | (0x1fU 
                                                                     & (vlSelf->__PVT__mem_rdata_q 
                                                                        >> 7U))))
                                                    : 0U)))));
        vlSelf->__PVT__instr_add = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0U == (0xfe007000U 
                                              & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sub = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x40000000U 
                                       == (0xfe007000U 
                                           & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sll = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x1000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_slt = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x2000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sltu = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                     & (0x3000U == 
                                        (0xfe007000U 
                                         & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_xor = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x4000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_srl = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x5000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sra = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x40005000U 
                                       == (0xfe007000U 
                                           & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_or = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                   & (0x6000U == (0xfe007000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_and = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x7000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__is_sll_srl_sra = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                         & ((IData)(
                                                    (0x1000U 
                                                     == 
                                                     (0xfe007000U 
                                                      & vlSelf->__PVT__mem_rdata_q))) 
                                            | ((IData)(
                                                       (0x5000U 
                                                        == 
                                                        (0xfe007000U 
                                                         & vlSelf->__PVT__mem_rdata_q))) 
                                               | (IData)(
                                                         (0x40005000U 
                                                          == 
                                                          (0xfe007000U 
                                                           & vlSelf->__PVT__mem_rdata_q))))));
    }
    if (((IData)(vlSelf->__PVT__mem_do_rinst) & (IData)(vlSelf->__PVT__mem_done))) {
        vlSelf->__PVT__compressed_instr = 0U;
        vlSelf->__PVT__is_alu_reg_imm = (0x13U == (0x7fU 
                                                   & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__is_lb_lh_lw_lbu_lhu = (3U == 
                                              (0x7fU 
                                               & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__is_sb_sh_sw = (0x23U == (0x7fU 
                                                & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__decoded_imm_j = ((0xfffffU & vlSelf->__PVT__decoded_imm_j) 
                                        | (0xfff00000U 
                                           & VL_EXTENDS_II(32,21, 
                                                           (0x1ffffeU 
                                                            & (vlSelf->__PVT__mem_rdata_latched 
                                                               >> 0xbU)))));
        vlSelf->__PVT__decoded_imm_j = ((0xfffff801U 
                                         & vlSelf->__PVT__decoded_imm_j) 
                                        | (0x7feU & 
                                           (VL_EXTENDS_II(32,21, 
                                                          (0x1ffffeU 
                                                           & (vlSelf->__PVT__mem_rdata_latched 
                                                              >> 0xbU))) 
                                            >> 9U)));
        vlSelf->__PVT__decoded_imm_j = ((0xfffff7ffU 
                                         & vlSelf->__PVT__decoded_imm_j) 
                                        | (0x800U & 
                                           (VL_EXTENDS_II(32,21, 
                                                          (0x1ffffeU 
                                                           & (vlSelf->__PVT__mem_rdata_latched 
                                                              >> 0xbU))) 
                                            << 2U)));
        vlSelf->__PVT__decoded_imm_j = ((0xfff00fffU 
                                         & vlSelf->__PVT__decoded_imm_j) 
                                        | (0xff000U 
                                           & (VL_EXTENDS_II(32,21, 
                                                            (0x1ffffeU 
                                                             & (vlSelf->__PVT__mem_rdata_latched 
                                                                >> 0xbU))) 
                                              << 0xbU)));
        vlSelf->__PVT__decoded_imm_j = ((0xfffffffeU 
                                         & vlSelf->__PVT__decoded_imm_j) 
                                        | (1U & VL_EXTENDS_II(1,21, 
                                                              (0x1ffffeU 
                                                               & (vlSelf->__PVT__mem_rdata_latched 
                                                                  >> 0xbU)))));
        vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu 
            = (0x63U == (0x7fU & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__instr_auipc = (0x17U == (0x7fU 
                                                & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__instr_lui = (0x37U == (0x7fU 
                                              & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__instr_jal = (0x6fU == (0x7fU 
                                              & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__instr_jalr = (IData)((0x67U 
                                             == (0x707fU 
                                                 & vlSelf->__PVT__mem_rdata_latched)));
        vlSelf->__PVT__is_alu_reg_reg = (0x33U == (0x7fU 
                                                   & vlSelf->__PVT__mem_rdata_latched));
    }
    if ((1U & (~ (IData)(vlSymsp->TOP.resetn)))) {
        vlSelf->__PVT__is_compare = 0U;
        vlSelf->__PVT__instr_beq = 0U;
        vlSelf->__PVT__instr_bne = 0U;
        vlSelf->__PVT__instr_blt = 0U;
        vlSelf->__PVT__instr_bge = 0U;
        vlSelf->__PVT__instr_bltu = 0U;
        vlSelf->__PVT__instr_bgeu = 0U;
        vlSelf->__PVT__instr_addi = 0U;
        vlSelf->__PVT__instr_slti = 0U;
        vlSelf->__PVT__instr_sltiu = 0U;
        vlSelf->__PVT__instr_xori = 0U;
        vlSelf->__PVT__instr_ori = 0U;
        vlSelf->__PVT__instr_andi = 0U;
        vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu = 0U;
        vlSelf->__PVT__instr_add = 0U;
        vlSelf->__PVT__instr_sub = 0U;
        vlSelf->__PVT__instr_sll = 0U;
        vlSelf->__PVT__instr_slt = 0U;
        vlSelf->__PVT__instr_sltu = 0U;
        vlSelf->__PVT__instr_xor = 0U;
        vlSelf->__PVT__instr_srl = 0U;
        vlSelf->__PVT__instr_sra = 0U;
        vlSelf->__PVT__instr_or = 0U;
        vlSelf->__PVT__instr_and = 0U;
    }
    vlSelf->__PVT__decoder_pseudo_trigger = __Vdly__decoder_pseudo_trigger;
    vlSelf->__PVT__decoder_trigger = __Vdly__decoder_trigger;
    if (vlSelf->__PVT__mem_xfer) {
        vlSelf->__PVT__mem_rdata_q = vlSymsp->TOP.mem_axi_rdata;
    }
    vlSelf->__PVT__alu_out_0 = 0U;
    if (vlSelf->__PVT__instr_beq) {
        vlSelf->__PVT__alu_out_0 = __PVT__alu_eq;
    } else if (vlSelf->__PVT__instr_bne) {
        vlSelf->__PVT__alu_out_0 = (1U & (~ (IData)(__PVT__alu_eq)));
    } else if (vlSelf->__PVT__instr_bge) {
        vlSelf->__PVT__alu_out_0 = (1U & (~ (IData)(__PVT__alu_lts)));
    } else if (vlSelf->__PVT__instr_bgeu) {
        vlSelf->__PVT__alu_out_0 = (1U & (~ (IData)(__PVT__alu_ltu)));
    } else if (vlSelf->__PVT__is_slti_blt_slt) {
        vlSelf->__PVT__alu_out_0 = __PVT__alu_lts;
    } else if (vlSelf->__PVT__is_sltiu_bltu_sltu) {
        vlSelf->__PVT__alu_out_0 = __PVT__alu_ltu;
    }
    vlSelf->__PVT__alu_out = 0U;
    if (vlSelf->__PVT__is_lui_auipc_jal_jalr_addi_add_sub) {
        vlSelf->__PVT__alu_out = ((IData)(vlSelf->__PVT__instr_sub)
                                   ? (vlSelf->__PVT__reg_op1 
                                      - vlSelf->__PVT__reg_op2)
                                   : (vlSelf->__PVT__reg_op1 
                                      + vlSelf->__PVT__reg_op2));
    } else if (vlSelf->__PVT__is_compare) {
        vlSelf->__PVT__alu_out = vlSelf->__PVT__alu_out_0;
    } else if (((IData)(vlSelf->__PVT__instr_xori) 
                | (IData)(vlSelf->__PVT__instr_xor))) {
        vlSelf->__PVT__alu_out = (vlSelf->__PVT__reg_op1 
                                  ^ vlSelf->__PVT__reg_op2);
    } else if (((IData)(vlSelf->__PVT__instr_ori) | (IData)(vlSelf->__PVT__instr_or))) {
        vlSelf->__PVT__alu_out = (vlSelf->__PVT__reg_op1 
                                  | vlSelf->__PVT__reg_op2);
    } else if (((IData)(vlSelf->__PVT__instr_andi) 
                | (IData)(vlSelf->__PVT__instr_and))) {
        vlSelf->__PVT__alu_out = (vlSelf->__PVT__reg_op1 
                                  & vlSelf->__PVT__reg_op2);
    }
    vlSelf->__PVT__instr_trap = (1U & (~ ((IData)(vlSelf->__PVT__instr_lui) 
                                          | ((IData)(vlSelf->__PVT__instr_auipc) 
                                             | ((IData)(vlSelf->__PVT__instr_jal) 
                                                | ((IData)(vlSelf->__PVT__instr_jalr) 
                                                   | ((IData)(vlSelf->__PVT__instr_beq) 
                                                      | ((IData)(vlSelf->__PVT__instr_bne) 
                                                         | ((IData)(vlSelf->__PVT__instr_blt) 
                                                            | ((IData)(vlSelf->__PVT__instr_bge) 
                                                               | ((IData)(vlSelf->__PVT__instr_bltu) 
                                                                  | ((IData)(vlSelf->__PVT__instr_bgeu) 
                                                                     | ((IData)(vlSelf->__PVT__instr_lb) 
                                                                        | ((IData)(vlSelf->__PVT__instr_lh) 
                                                                           | ((IData)(vlSelf->__PVT__instr_lw) 
                                                                              | ((IData)(vlSelf->__PVT__instr_lbu) 
                                                                                | ((IData)(vlSelf->__PVT__instr_lhu) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sb) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sh) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sw) 
                                                                                | ((IData)(vlSelf->__PVT__instr_addi) 
                                                                                | ((IData)(vlSelf->__PVT__instr_slti) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sltiu) 
                                                                                | ((IData)(vlSelf->__PVT__instr_xori) 
                                                                                | ((IData)(vlSelf->__PVT__instr_ori) 
                                                                                | ((IData)(vlSelf->__PVT__instr_andi) 
                                                                                | ((IData)(vlSelf->__PVT__instr_slli) 
                                                                                | ((IData)(vlSelf->__PVT__instr_srli) 
                                                                                | ((IData)(vlSelf->__PVT__instr_srai) 
                                                                                | ((IData)(vlSelf->__PVT__instr_add) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sub) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sll) 
                                                                                | ((IData)(vlSelf->__PVT__instr_slt) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sltu) 
                                                                                | ((IData)(vlSelf->__PVT__instr_xor) 
                                                                                | ((IData)(vlSelf->__PVT__instr_srl) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sra) 
                                                                                | ((IData)(vlSelf->__PVT__instr_or) 
                                                                                | ((IData)(vlSelf->__PVT__instr_and) 
                                                                                | ((IData)(vlSelf->__PVT__instr_rdcycle) 
                                                                                | ((IData)(vlSelf->__PVT__instr_rdcycleh) 
                                                                                | ((IData)(vlSelf->__PVT__instr_rdinstr) 
                                                                                | ((IData)(vlSelf->__PVT__instr_rdinstrh) 
                                                                                | ((IData)(vlSelf->__PVT__instr_fence) 
                                                                                | ((IData)(vlSelf->__PVT__instr_getq) 
                                                                                | ((IData)(vlSelf->__PVT__instr_setq) 
                                                                                | ((IData)(vlSelf->__PVT__compressed_instr) 
                                                                                | ((IData)(vlSelf->__PVT__instr_maskirq) 
                                                                                | ((IData)(vlSelf->__PVT__instr_timer) 
                                                                                | (IData)(vlSelf->__PVT__instr_waitirq))))))))))))))))))))))))))))))))))))))))))))))))));
    vlSelf->__PVT__mem_do_rinst = vlSelf->__Vdly__mem_do_rinst;
    vlSelf->__PVT__mem_la_read = ((IData)(vlSymsp->TOP.resetn) 
                                  & ((~ (IData)((0U 
                                                 != (IData)(vlSelf->__PVT__mem_state)))) 
                                     & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                        | ((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                           | (IData)(vlSelf->__PVT__mem_do_rdata)))));
}

VL_INLINE_OPT void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__2(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_axi__DOT__picorv32_core__2\n"); );
    // Body
    vlSelf->__PVT__mem_xfer = ((IData)(vlSymsp->TOP.picorv32_axi__DOT__mem_ready) 
                               & (IData)(vlSelf->mem_valid));
    vlSelf->__PVT__mem_rdata_latched = ((IData)(vlSelf->__PVT__mem_xfer)
                                         ? vlSymsp->TOP.mem_axi_rdata
                                         : vlSelf->__PVT__mem_rdata_q);
    vlSelf->__PVT__mem_done = ((IData)(vlSymsp->TOP.resetn) 
                               & (((IData)(vlSelf->__PVT__mem_xfer) 
                                   & ((0U != (IData)(vlSelf->__PVT__mem_state)) 
                                      & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                         | ((IData)(vlSelf->__PVT__mem_do_rdata) 
                                            | (IData)(vlSelf->__PVT__mem_do_wdata))))) 
                                  | ((3U == (IData)(vlSelf->__PVT__mem_state)) 
                                     & (IData)(vlSelf->__PVT__mem_do_rinst))));
}

VL_INLINE_OPT void Vpicorv32_picorv32__pi1___ico_sequent__TOP__picorv32_wb__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___ico_sequent__TOP__picorv32_wb__DOT__picorv32_core__0\n"); );
    // Body
    vlSelf->__PVT__mem_la_write = ((~ (IData)(vlSymsp->TOP.wb_rst_i)) 
                                   & ((~ (IData)((0U 
                                                  != (IData)(vlSelf->__PVT__mem_state)))) 
                                      & (IData)(vlSelf->__PVT__mem_do_wdata)));
    vlSelf->__PVT__mem_la_read = ((~ (IData)(vlSymsp->TOP.wb_rst_i)) 
                                  & ((~ (IData)((0U 
                                                 != (IData)(vlSelf->__PVT__mem_state)))) 
                                     & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                        | ((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                           | (IData)(vlSelf->__PVT__mem_do_rdata)))));
    vlSelf->__PVT__mem_done = ((~ (IData)(vlSymsp->TOP.wb_rst_i)) 
                               & (((IData)(vlSelf->__PVT__mem_xfer) 
                                   & ((0U != (IData)(vlSelf->__PVT__mem_state)) 
                                      & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                         | ((IData)(vlSelf->__PVT__mem_do_rdata) 
                                            | (IData)(vlSelf->__PVT__mem_do_wdata))))) 
                                  | ((3U == (IData)(vlSelf->__PVT__mem_state)) 
                                     & (IData)(vlSelf->__PVT__mem_do_rinst))));
}

VL_INLINE_OPT void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__0\n"); );
    // Init
    CData/*4:0*/ __Vdlyvdim0__cpuregs__v0;
    __Vdlyvdim0__cpuregs__v0 = 0;
    IData/*31:0*/ __Vdlyvval__cpuregs__v0;
    __Vdlyvval__cpuregs__v0 = 0;
    CData/*0:0*/ __Vdlyvset__cpuregs__v0;
    __Vdlyvset__cpuregs__v0 = 0;
    CData/*4:0*/ __Vdly__reg_sh;
    __Vdly__reg_sh = 0;
    IData/*31:0*/ __Vdly__reg_out;
    __Vdly__reg_out = 0;
    QData/*63:0*/ __Vdly__count_cycle;
    __Vdly__count_cycle = 0;
    CData/*0:0*/ __Vdly__decoder_trigger;
    __Vdly__decoder_trigger = 0;
    CData/*0:0*/ __Vdly__decoder_pseudo_trigger;
    __Vdly__decoder_pseudo_trigger = 0;
    // Body
    vlSelf->__Vdly__timer = vlSelf->__PVT__timer;
    vlSelf->__Vdly__latched_is_lb = vlSelf->__PVT__latched_is_lb;
    vlSelf->__Vdly__latched_is_lh = vlSelf->__PVT__latched_is_lh;
    vlSelf->__Vdly__latched_is_lu = vlSelf->__PVT__latched_is_lu;
    vlSelf->__Vdly__count_instr = vlSelf->__PVT__count_instr;
    __Vdly__decoder_pseudo_trigger = vlSelf->__PVT__decoder_pseudo_trigger;
    __Vdly__count_cycle = vlSelf->__PVT__count_cycle;
    __Vdly__reg_sh = vlSelf->__PVT__reg_sh;
    __Vdly__decoder_trigger = vlSelf->__PVT__decoder_trigger;
    vlSelf->__Vdly__reg_next_pc = vlSelf->__PVT__reg_next_pc;
    vlSelf->__Vdly__mem_do_prefetch = vlSelf->__PVT__mem_do_prefetch;
    vlSelf->__Vdly__latched_stalu = vlSelf->__PVT__latched_stalu;
    vlSelf->__Vdly__reg_pc = vlSelf->__PVT__reg_pc;
    vlSelf->__Vdly__mem_wordsize = vlSelf->__PVT__mem_wordsize;
    vlSelf->__Vdly__mem_do_rinst = vlSelf->__PVT__mem_do_rinst;
    __Vdly__reg_out = vlSelf->__PVT__reg_out;
    vlSelf->__Vdly__cpu_state = vlSelf->__PVT__cpu_state;
    vlSelf->__Vdly__latched_branch = vlSelf->__PVT__latched_branch;
    vlSelf->__Vdly__latched_store = vlSelf->__PVT__latched_store;
    __Vdlyvset__cpuregs__v0 = 0U;
    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__reg_op1;
    vlSelf->__Vdly__mem_state = vlSelf->__PVT__mem_state;
    if ((((~ (IData)(vlSymsp->TOP.wb_rst_i)) & (IData)(vlSelf->__PVT__cpuregs_write)) 
         & (0U != (IData)(vlSelf->__PVT__latched_rd)))) {
        __Vdlyvval__cpuregs__v0 = vlSelf->__PVT__cpuregs_wrdata;
        __Vdlyvset__cpuregs__v0 = 1U;
        __Vdlyvdim0__cpuregs__v0 = vlSelf->__PVT__latched_rd;
    }
    vlSelf->trace_data = 0ULL;
    if (((IData)(vlSelf->__PVT__decoder_trigger) & 
         (~ (IData)(vlSelf->__PVT__decoder_pseudo_trigger)))) {
        vlSelf->pcpi_insn = 0U;
        vlSelf->__PVT__instr_setq = 0U;
        vlSelf->__PVT__instr_getq = 0U;
        vlSelf->__PVT__instr_maskirq = 0U;
        vlSelf->__PVT__instr_timer = 0U;
        vlSelf->__PVT__instr_fence = ((0xfU == (0x7fU 
                                                & vlSelf->__PVT__mem_rdata_q)) 
                                      & (~ (IData)(
                                                   (0U 
                                                    != 
                                                    (7U 
                                                     & (vlSelf->__PVT__mem_rdata_q 
                                                        >> 0xcU))))));
    }
    if (vlSymsp->TOP.wb_rst_i) {
        vlSelf->eoi = 0U;
        vlSelf->__PVT__irq_active = 0U;
        vlSelf->__PVT__instr_fence = 0U;
    }
    if (((IData)(vlSelf->__PVT__mem_do_rinst) & (IData)(vlSelf->__PVT__mem_done))) {
        vlSelf->__PVT__decoded_rs1 = (0x1fU & (vlSelf->__PVT__mem_rdata_latched 
                                               >> 0xfU));
        vlSelf->__PVT__instr_waitirq = 0U;
    }
    if (__Vdlyvset__cpuregs__v0) {
        vlSelf->__PVT__cpuregs[__Vdlyvdim0__cpuregs__v0] 
            = __Vdlyvval__cpuregs__v0;
    }
}

VL_INLINE_OPT void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__1(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__1\n"); );
    // Init
    CData/*0:0*/ __PVT__set_mem_do_rinst;
    __PVT__set_mem_do_rinst = 0;
    CData/*0:0*/ __PVT__set_mem_do_rdata;
    __PVT__set_mem_do_rdata = 0;
    CData/*0:0*/ __PVT__set_mem_do_wdata;
    __PVT__set_mem_do_wdata = 0;
    CData/*0:0*/ __PVT__alu_eq;
    __PVT__alu_eq = 0;
    CData/*0:0*/ __PVT__alu_ltu;
    __PVT__alu_ltu = 0;
    CData/*0:0*/ __PVT__alu_lts;
    __PVT__alu_lts = 0;
    CData/*4:0*/ __Vdly__reg_sh;
    __Vdly__reg_sh = 0;
    IData/*31:0*/ __Vdly__reg_out;
    __Vdly__reg_out = 0;
    QData/*63:0*/ __Vdly__count_cycle;
    __Vdly__count_cycle = 0;
    CData/*0:0*/ __Vdly__decoder_trigger;
    __Vdly__decoder_trigger = 0;
    CData/*0:0*/ __Vdly__decoder_pseudo_trigger;
    __Vdly__decoder_pseudo_trigger = 0;
    // Body
    if ((1U & (~ ((IData)(vlSymsp->TOP.wb_rst_i) | (IData)(vlSelf->trap))))) {
        if (vlSelf->__PVT__mem_la_write) {
            vlSelf->mem_wdata = vlSelf->__PVT__mem_la_wdata;
        }
        if (((IData)(vlSelf->__PVT__mem_la_read) | (IData)(vlSelf->__PVT__mem_la_write))) {
            vlSelf->mem_addr = (((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                 | (IData)(vlSelf->__PVT__mem_do_rinst))
                                 ? (0xfffffffcU & vlSelf->__PVT__next_pc)
                                 : (0xfffffffcU & vlSelf->__PVT__reg_op1));
        }
    }
    if (((IData)(vlSymsp->TOP.wb_rst_i) | (IData)(vlSelf->trap))) {
        if (vlSymsp->TOP.wb_rst_i) {
            vlSelf->__Vdly__mem_state = 0U;
        }
        if (((IData)(vlSymsp->TOP.wb_rst_i) | (IData)(vlSymsp->TOP.picorv32_wb__DOT__mem_ready))) {
            vlSelf->mem_valid = 0U;
        }
    } else {
        if (((IData)(vlSelf->__PVT__mem_la_read) | (IData)(vlSelf->__PVT__mem_la_write))) {
            vlSelf->mem_wstrb = ((IData)(vlSelf->__PVT__mem_la_wstrb) 
                                 & (- (IData)((IData)(vlSelf->__PVT__mem_la_write))));
        }
        if ((0U == (IData)(vlSelf->__PVT__mem_state))) {
            if ((((IData)(vlSelf->__PVT__mem_do_prefetch) 
                  | (IData)(vlSelf->__PVT__mem_do_rinst)) 
                 | (IData)(vlSelf->__PVT__mem_do_rdata))) {
                vlSelf->mem_valid = 1U;
                vlSelf->mem_instr = ((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                     | (IData)(vlSelf->__PVT__mem_do_rinst));
                vlSelf->mem_wstrb = 0U;
                vlSelf->__Vdly__mem_state = 1U;
            }
            if (vlSelf->__PVT__mem_do_wdata) {
                vlSelf->mem_valid = 1U;
                vlSelf->mem_instr = 0U;
                vlSelf->__Vdly__mem_state = 2U;
            }
        } else if ((1U == (IData)(vlSelf->__PVT__mem_state))) {
            if (vlSelf->__PVT__mem_xfer) {
                vlSelf->mem_valid = 0U;
                vlSelf->__Vdly__mem_state = (((IData)(vlSelf->__PVT__mem_do_rinst) 
                                              | (IData)(vlSelf->__PVT__mem_do_rdata))
                                              ? 0U : 3U);
            }
        } else if ((2U == (IData)(vlSelf->__PVT__mem_state))) {
            if (vlSelf->__PVT__mem_xfer) {
                vlSelf->mem_valid = 0U;
                vlSelf->__Vdly__mem_state = 0U;
            }
        } else if ((3U == (IData)(vlSelf->__PVT__mem_state))) {
            if (vlSelf->__PVT__mem_do_rinst) {
                vlSelf->__Vdly__mem_state = 0U;
            }
        }
    }
    vlSelf->__PVT__mem_state = vlSelf->__Vdly__mem_state;
    vlSelf->trap = 0U;
    __Vdly__reg_sh = 0U;
    __Vdly__reg_out = 0U;
    __PVT__set_mem_do_rinst = 0U;
    __PVT__set_mem_do_rdata = 0U;
    __PVT__set_mem_do_wdata = 0U;
    __Vdly__decoder_trigger = ((IData)(vlSelf->__PVT__mem_do_rinst) 
                               & (IData)(vlSelf->__PVT__mem_done));
    __Vdly__decoder_pseudo_trigger = 0U;
    if (vlSymsp->TOP.wb_rst_i) {
        __Vdly__count_cycle = 0ULL;
        vlSelf->__Vdly__reg_pc = 0U;
        vlSelf->__Vdly__reg_next_pc = 0U;
        vlSelf->__Vdly__count_instr = 0ULL;
        vlSelf->__Vdly__latched_store = 0U;
        vlSelf->__Vdly__latched_stalu = 0U;
        vlSelf->__Vdly__latched_branch = 0U;
        vlSelf->__Vdly__latched_is_lu = 0U;
        vlSelf->__Vdly__latched_is_lh = 0U;
        vlSelf->__Vdly__latched_is_lb = 0U;
        vlSelf->__Vdly__timer = 0U;
        vlSelf->__Vdly__cpu_state = 0x40U;
    } else {
        __Vdly__count_cycle = (1ULL + vlSelf->__PVT__count_cycle);
        if (((((((((0x80U == (IData)(vlSelf->__PVT__cpu_state)) 
                   | (0x40U == (IData)(vlSelf->__PVT__cpu_state))) 
                  | (0x20U == (IData)(vlSelf->__PVT__cpu_state))) 
                 | (0x10U == (IData)(vlSelf->__PVT__cpu_state))) 
                | (8U == (IData)(vlSelf->__PVT__cpu_state))) 
               | (4U == (IData)(vlSelf->__PVT__cpu_state))) 
              | (2U == (IData)(vlSelf->__PVT__cpu_state))) 
             | (1U == (IData)(vlSelf->__PVT__cpu_state)))) {
            if ((0x80U == (IData)(vlSelf->__PVT__cpu_state))) {
                vlSelf->trap = 1U;
            } else if ((0x40U == (IData)(vlSelf->__PVT__cpu_state))) {
                vlSelf->__Vdly__mem_do_rinst = (1U 
                                                & ((~ (IData)(vlSelf->__PVT__decoder_trigger)) 
                                                   & (~ (IData)(vlSelf->__PVT__do_waitirq))));
                vlSelf->__Vdly__mem_wordsize = 0U;
                vlSelf->__Vdly__latched_is_lu = 0U;
                vlSelf->__Vdly__latched_is_lh = 0U;
                vlSelf->__Vdly__latched_is_lb = 0U;
                vlSelf->__PVT__latched_rd = vlSelf->__PVT__decoded_rd;
                vlSelf->__PVT__latched_compr = vlSelf->__PVT__compressed_instr;
                vlSelf->__PVT__current_pc = vlSelf->__PVT__reg_next_pc;
                if (vlSelf->__PVT__latched_branch) {
                    vlSelf->__PVT__current_pc = ((IData)(vlSelf->__PVT__latched_store)
                                                  ? 
                                                 (0xfffffffeU 
                                                  & ((IData)(vlSelf->__PVT__latched_stalu)
                                                      ? vlSelf->__PVT__alu_out_q
                                                      : vlSelf->__PVT__reg_out))
                                                  : vlSelf->__PVT__reg_next_pc);
                }
                vlSelf->__Vdly__reg_pc = vlSelf->__PVT__current_pc;
                vlSelf->__Vdly__reg_next_pc = vlSelf->__PVT__current_pc;
                vlSelf->__Vdly__latched_store = 0U;
                vlSelf->__Vdly__latched_stalu = 0U;
                vlSelf->__Vdly__latched_branch = 0U;
                if (vlSelf->__PVT__decoder_trigger) {
                    vlSelf->__Vdly__count_instr = (1ULL 
                                                   + vlSelf->__PVT__count_instr);
                    vlSelf->__Vdly__reg_next_pc = (vlSelf->__PVT__current_pc 
                                                   + 
                                                   ((IData)(vlSelf->__PVT__compressed_instr)
                                                     ? 2U
                                                     : 4U));
                    if (vlSelf->__PVT__instr_jal) {
                        vlSelf->__Vdly__mem_do_rinst = 1U;
                        vlSelf->__Vdly__reg_next_pc 
                            = (vlSelf->__PVT__current_pc 
                               + vlSelf->__PVT__decoded_imm_j);
                        vlSelf->__Vdly__latched_branch = 1U;
                    } else {
                        vlSelf->__Vdly__mem_do_rinst = 0U;
                        vlSelf->__Vdly__mem_do_prefetch 
                            = (1U & ((~ (IData)(vlSelf->__PVT__instr_jalr)) 
                                     & (~ (IData)(vlSelf->__PVT__compressed_instr))));
                        vlSelf->__Vdly__cpu_state = 0x20U;
                    }
                }
            } else if ((0x20U == (IData)(vlSelf->__PVT__cpu_state))) {
                vlSelf->__Vdly__reg_op1 = 0U;
                vlSelf->__PVT__reg_op2 = 0U;
                if ((((IData)(vlSelf->__PVT__instr_trap) 
                      | (IData)(vlSelf->__PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh)) 
                     | (IData)(vlSelf->__PVT__is_lui_auipc_jal))) {
                    if (vlSelf->__PVT__instr_trap) {
                        vlSelf->__Vdly__cpu_state = 0x80U;
                    } else if (vlSelf->__PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh) {
                        if (vlSelf->__PVT__instr_rdcycle) {
                            __Vdly__reg_out = (IData)(vlSelf->__PVT__count_cycle);
                        } else if (vlSelf->__PVT__instr_rdcycleh) {
                            __Vdly__reg_out = (IData)(
                                                      (vlSelf->__PVT__count_cycle 
                                                       >> 0x20U));
                        } else if (vlSelf->__PVT__instr_rdinstr) {
                            __Vdly__reg_out = (IData)(vlSelf->__PVT__count_instr);
                        } else if (vlSelf->__PVT__instr_rdinstrh) {
                            __Vdly__reg_out = (IData)(
                                                      (vlSelf->__PVT__count_instr 
                                                       >> 0x20U));
                        }
                        vlSelf->__Vdly__latched_store = 1U;
                        vlSelf->__Vdly__cpu_state = 0x40U;
                    } else if (vlSelf->__PVT__is_lui_auipc_jal) {
                        vlSelf->__Vdly__reg_op1 = ((IData)(vlSelf->__PVT__instr_lui)
                                                    ? 0U
                                                    : vlSelf->__PVT__reg_pc);
                        vlSelf->__PVT__reg_op2 = vlSelf->__PVT__decoded_imm;
                        vlSelf->__Vdly__mem_do_rinst 
                            = vlSelf->__PVT__mem_do_prefetch;
                        vlSelf->__Vdly__cpu_state = 8U;
                    } else {
                        vlSelf->__Vdly__latched_store = 1U;
                        __Vdly__reg_out = vlSelf->__PVT__timer;
                        vlSelf->__Vdly__cpu_state = 0x40U;
                        vlSelf->__Vdly__timer = vlSelf->__PVT__cpuregs_rs1;
                    }
                } else if (((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                            & (~ (IData)(vlSelf->__PVT__instr_trap)))) {
                    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__cpuregs_rs1;
                    vlSelf->__Vdly__cpu_state = 1U;
                    vlSelf->__Vdly__mem_do_rinst = 1U;
                } else if (vlSelf->__PVT__is_slli_srli_srai) {
                    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__cpuregs_rs1;
                    __Vdly__reg_sh = vlSelf->__PVT__decoded_rs2;
                    vlSelf->__Vdly__cpu_state = 4U;
                } else if (vlSelf->__PVT__is_jalr_addi_slti_sltiu_xori_ori_andi) {
                    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__cpuregs_rs1;
                    vlSelf->__PVT__reg_op2 = vlSelf->__PVT__decoded_imm;
                    vlSelf->__Vdly__mem_do_rinst = vlSelf->__PVT__mem_do_prefetch;
                    vlSelf->__Vdly__cpu_state = 8U;
                } else {
                    vlSelf->__Vdly__reg_op1 = vlSelf->__PVT__cpuregs_rs1;
                    __Vdly__reg_sh = (0x1fU & vlSelf->__PVT__cpuregs_rs2);
                    vlSelf->__PVT__reg_op2 = vlSelf->__PVT__cpuregs_rs2;
                    if (vlSelf->__PVT__is_sb_sh_sw) {
                        vlSelf->__Vdly__cpu_state = 2U;
                        vlSelf->__Vdly__mem_do_rinst = 1U;
                    } else if (vlSelf->__PVT__is_sll_srl_sra) {
                        vlSelf->__Vdly__cpu_state = 4U;
                    } else {
                        vlSelf->__Vdly__mem_do_rinst 
                            = vlSelf->__PVT__mem_do_prefetch;
                        vlSelf->__Vdly__cpu_state = 8U;
                    }
                }
            } else if ((0x10U == (IData)(vlSelf->__PVT__cpu_state))) {
                __Vdly__reg_sh = (0x1fU & vlSelf->__PVT__cpuregs_rs2);
                vlSelf->__PVT__reg_op2 = vlSelf->__PVT__cpuregs_rs2;
                if (vlSelf->__PVT__is_sb_sh_sw) {
                    vlSelf->__Vdly__cpu_state = 2U;
                    vlSelf->__Vdly__mem_do_rinst = 1U;
                } else if (vlSelf->__PVT__is_sll_srl_sra) {
                    vlSelf->__Vdly__cpu_state = 4U;
                } else {
                    vlSelf->__Vdly__mem_do_rinst = vlSelf->__PVT__mem_do_prefetch;
                    vlSelf->__Vdly__cpu_state = 8U;
                }
            } else if ((8U == (IData)(vlSelf->__PVT__cpu_state))) {
                __Vdly__reg_out = (vlSelf->__PVT__reg_pc 
                                   + vlSelf->__PVT__decoded_imm);
                if (vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) {
                    vlSelf->__PVT__latched_rd = 0U;
                    vlSelf->__Vdly__latched_store = vlSelf->__PVT__alu_out_0;
                    vlSelf->__Vdly__latched_branch 
                        = vlSelf->__PVT__alu_out_0;
                    if (vlSelf->__PVT__mem_done) {
                        vlSelf->__Vdly__cpu_state = 0x40U;
                    }
                    if (vlSelf->__PVT__alu_out_0) {
                        __PVT__set_mem_do_rinst = 1U;
                        __Vdly__decoder_trigger = 0U;
                    }
                } else {
                    vlSelf->__Vdly__latched_branch 
                        = vlSelf->__PVT__instr_jalr;
                    vlSelf->__Vdly__latched_store = 1U;
                    vlSelf->__Vdly__latched_stalu = 1U;
                    vlSelf->__Vdly__cpu_state = 0x40U;
                }
            } else if ((4U == (IData)(vlSelf->__PVT__cpu_state))) {
                vlSelf->__Vdly__latched_store = 1U;
                if ((0U == (IData)(vlSelf->__PVT__reg_sh))) {
                    __Vdly__reg_out = vlSelf->__PVT__reg_op1;
                    vlSelf->__Vdly__mem_do_rinst = vlSelf->__PVT__mem_do_prefetch;
                    vlSelf->__Vdly__cpu_state = 0x40U;
                } else if ((4U <= (IData)(vlSelf->__PVT__reg_sh))) {
                    if (((IData)(vlSelf->__PVT__instr_slli) 
                         | (IData)(vlSelf->__PVT__instr_sll))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTL_III(32,32,32, vlSelf->__PVT__reg_op1, 4U);
                    } else if (((IData)(vlSelf->__PVT__instr_srli) 
                                | (IData)(vlSelf->__PVT__instr_srl))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTR_III(32,32,32, vlSelf->__PVT__reg_op1, 4U);
                    } else if (((IData)(vlSelf->__PVT__instr_srai) 
                                | (IData)(vlSelf->__PVT__instr_sra))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTRS_III(32,32,32, vlSelf->__PVT__reg_op1, 4U);
                    }
                    __Vdly__reg_sh = (0x1fU & ((IData)(vlSelf->__PVT__reg_sh) 
                                               - (IData)(4U)));
                } else {
                    if (((IData)(vlSelf->__PVT__instr_slli) 
                         | (IData)(vlSelf->__PVT__instr_sll))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTL_III(32,32,32, vlSelf->__PVT__reg_op1, 1U);
                    } else if (((IData)(vlSelf->__PVT__instr_srli) 
                                | (IData)(vlSelf->__PVT__instr_srl))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTR_III(32,32,32, vlSelf->__PVT__reg_op1, 1U);
                    } else if (((IData)(vlSelf->__PVT__instr_srai) 
                                | (IData)(vlSelf->__PVT__instr_sra))) {
                        vlSelf->__Vdly__reg_op1 = VL_SHIFTRS_III(32,32,32, vlSelf->__PVT__reg_op1, 1U);
                    }
                    __Vdly__reg_sh = (0x1fU & ((IData)(vlSelf->__PVT__reg_sh) 
                                               - (IData)(1U)));
                }
            } else if ((2U == (IData)(vlSelf->__PVT__cpu_state))) {
                if ((1U & ((~ (IData)(vlSelf->__PVT__mem_do_prefetch)) 
                           | (IData)(vlSelf->__PVT__mem_done)))) {
                    if ((1U & (~ (IData)(vlSelf->__PVT__mem_do_wdata)))) {
                        vlSelf->__Vdly__reg_op1 = (vlSelf->__PVT__reg_op1 
                                                   + vlSelf->__PVT__decoded_imm);
                        __PVT__set_mem_do_wdata = 1U;
                        if (vlSelf->__PVT__instr_sb) {
                            vlSelf->__Vdly__mem_wordsize = 2U;
                        } else if (vlSelf->__PVT__instr_sh) {
                            vlSelf->__Vdly__mem_wordsize = 1U;
                        } else if (vlSelf->__PVT__instr_sw) {
                            vlSelf->__Vdly__mem_wordsize = 0U;
                        }
                    }
                    if (((~ (IData)(vlSelf->__PVT__mem_do_prefetch)) 
                         & (IData)(vlSelf->__PVT__mem_done))) {
                        vlSelf->__Vdly__cpu_state = 0x40U;
                        __Vdly__decoder_trigger = 1U;
                        __Vdly__decoder_pseudo_trigger = 1U;
                    }
                }
            } else {
                vlSelf->__Vdly__latched_store = 1U;
                if ((1U & ((~ (IData)(vlSelf->__PVT__mem_do_prefetch)) 
                           | (IData)(vlSelf->__PVT__mem_done)))) {
                    if (((~ (IData)(vlSelf->__PVT__mem_do_prefetch)) 
                         & (IData)(vlSelf->__PVT__mem_done))) {
                        if (vlSelf->__PVT__latched_is_lu) {
                            __Vdly__reg_out = vlSelf->__PVT__mem_rdata_word;
                        } else if (vlSelf->__PVT__latched_is_lh) {
                            __Vdly__reg_out = VL_EXTENDS_II(32,16, 
                                                            (0xffffU 
                                                             & vlSelf->__PVT__mem_rdata_word));
                        } else if (vlSelf->__PVT__latched_is_lb) {
                            __Vdly__reg_out = VL_EXTENDS_II(32,8, 
                                                            (0xffU 
                                                             & vlSelf->__PVT__mem_rdata_word));
                        }
                        __Vdly__decoder_trigger = 1U;
                        __Vdly__decoder_pseudo_trigger = 1U;
                        vlSelf->__Vdly__cpu_state = 0x40U;
                    }
                    if ((1U & (~ (IData)(vlSelf->__PVT__mem_do_rdata)))) {
                        vlSelf->__Vdly__reg_op1 = (vlSelf->__PVT__reg_op1 
                                                   + vlSelf->__PVT__decoded_imm);
                        __PVT__set_mem_do_rdata = 1U;
                        if (((IData)(vlSelf->__PVT__instr_lb) 
                             | (IData)(vlSelf->__PVT__instr_lbu))) {
                            vlSelf->__Vdly__mem_wordsize = 2U;
                        } else if (((IData)(vlSelf->__PVT__instr_lh) 
                                    | (IData)(vlSelf->__PVT__instr_lhu))) {
                            vlSelf->__Vdly__mem_wordsize = 1U;
                        } else if (vlSelf->__PVT__instr_lw) {
                            vlSelf->__Vdly__mem_wordsize = 0U;
                        }
                        vlSelf->__Vdly__latched_is_lu 
                            = vlSelf->__PVT__is_lbu_lhu_lw;
                        vlSelf->__Vdly__latched_is_lh 
                            = vlSelf->__PVT__instr_lh;
                        vlSelf->__Vdly__latched_is_lb 
                            = vlSelf->__PVT__instr_lb;
                    }
                }
            }
        }
    }
    if (((~ (IData)(vlSymsp->TOP.wb_rst_i)) & ((IData)(vlSelf->__PVT__mem_do_rdata) 
                                               | (IData)(vlSelf->__PVT__mem_do_wdata)))) {
        if (((0U == (IData)(vlSelf->__PVT__mem_wordsize)) 
             & (0U != (3U & vlSelf->__PVT__reg_op1)))) {
            vlSelf->__Vdly__cpu_state = 0x80U;
        }
        if (((1U == (IData)(vlSelf->__PVT__mem_wordsize)) 
             & vlSelf->__PVT__reg_op1)) {
            vlSelf->__Vdly__cpu_state = 0x80U;
        }
    }
    if ((((~ (IData)(vlSymsp->TOP.wb_rst_i)) & (IData)(vlSelf->__PVT__mem_do_rinst)) 
         & (0U != (3U & vlSelf->__PVT__reg_pc)))) {
        vlSelf->__Vdly__cpu_state = 0x80U;
    }
    if (((IData)(vlSymsp->TOP.wb_rst_i) | (IData)(vlSelf->__PVT__mem_done))) {
        vlSelf->__Vdly__mem_do_prefetch = 0U;
        vlSelf->__Vdly__mem_do_rinst = 0U;
        vlSelf->__PVT__mem_do_rdata = 0U;
        vlSelf->__PVT__mem_do_wdata = 0U;
    }
    if (__PVT__set_mem_do_rinst) {
        vlSelf->__Vdly__mem_do_rinst = 1U;
    }
    if (__PVT__set_mem_do_rdata) {
        vlSelf->__PVT__mem_do_rdata = 1U;
    }
    if (__PVT__set_mem_do_wdata) {
        vlSelf->__PVT__mem_do_wdata = 1U;
    }
    vlSelf->__PVT__current_pc = 0U;
    vlSelf->__PVT__reg_sh = __Vdly__reg_sh;
    vlSelf->__PVT__count_cycle = __Vdly__count_cycle;
    vlSelf->__PVT__count_instr = vlSelf->__Vdly__count_instr;
    vlSelf->__PVT__latched_is_lu = vlSelf->__Vdly__latched_is_lu;
    vlSelf->__PVT__latched_is_lh = vlSelf->__Vdly__latched_is_lh;
    vlSelf->__PVT__latched_is_lb = vlSelf->__Vdly__latched_is_lb;
    vlSelf->__PVT__timer = vlSelf->__Vdly__timer;
    vlSelf->__PVT__cpuregs_rs1 = ((0U != (IData)(vlSelf->__PVT__decoded_rs1))
                                   ? vlSelf->__PVT__cpuregs
                                  [vlSelf->__PVT__decoded_rs1]
                                   : 0U);
    vlSelf->__PVT__reg_next_pc = vlSelf->__Vdly__reg_next_pc;
    vlSelf->__PVT__mem_do_prefetch = vlSelf->__Vdly__mem_do_prefetch;
    vlSelf->__PVT__reg_pc = vlSelf->__Vdly__reg_pc;
    vlSelf->__PVT__latched_stalu = vlSelf->__Vdly__latched_stalu;
    vlSelf->__PVT__mem_wordsize = vlSelf->__Vdly__mem_wordsize;
    vlSelf->__PVT__reg_out = __Vdly__reg_out;
    vlSelf->__PVT__cpu_state = vlSelf->__Vdly__cpu_state;
    vlSelf->__PVT__latched_store = vlSelf->__Vdly__latched_store;
    vlSelf->__PVT__latched_branch = vlSelf->__Vdly__latched_branch;
    vlSelf->__PVT__reg_op1 = vlSelf->__Vdly__reg_op1;
    vlSelf->__PVT__mem_la_write = ((~ (IData)(vlSymsp->TOP.wb_rst_i)) 
                                   & ((~ (IData)((0U 
                                                  != (IData)(vlSelf->__PVT__mem_state)))) 
                                      & (IData)(vlSelf->__PVT__mem_do_wdata)));
    vlSelf->__PVT__do_waitirq = 0U;
    vlSelf->__PVT__cpuregs_write = 0U;
    vlSelf->__PVT__next_pc = (((IData)(vlSelf->__PVT__latched_branch) 
                               & (IData)(vlSelf->__PVT__latched_store))
                               ? (0xfffffffeU & vlSelf->__PVT__reg_out)
                               : vlSelf->__PVT__reg_next_pc);
    vlSelf->__PVT__alu_out_q = vlSelf->__PVT__alu_out;
    if ((0U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_la_wdata = vlSelf->__PVT__reg_op2;
        vlSelf->__PVT__mem_la_wstrb = 0xfU;
    } else if ((1U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_la_wdata = ((vlSelf->__PVT__reg_op2 
                                        << 0x10U) | 
                                       (0xffffU & vlSelf->__PVT__reg_op2));
        vlSelf->__PVT__mem_la_wstrb = ((2U & vlSelf->__PVT__reg_op1)
                                        ? 0xcU : 3U);
    } else if ((2U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_la_wdata = ((vlSelf->__PVT__reg_op2 
                                        << 0x18U) | 
                                       ((0xff0000U 
                                         & (vlSelf->__PVT__reg_op2 
                                            << 0x10U)) 
                                        | ((0xff00U 
                                            & (vlSelf->__PVT__reg_op2 
                                               << 8U)) 
                                           | (0xffU 
                                              & vlSelf->__PVT__reg_op2))));
        vlSelf->__PVT__mem_la_wstrb = (0xfU & ((IData)(1U) 
                                               << (3U 
                                                   & vlSelf->__PVT__reg_op1)));
    }
    __PVT__alu_eq = (vlSelf->__PVT__reg_op1 == vlSelf->__PVT__reg_op2);
    __PVT__alu_lts = VL_LTS_III(32, vlSelf->__PVT__reg_op1, vlSelf->__PVT__reg_op2);
    __PVT__alu_ltu = (vlSelf->__PVT__reg_op1 < vlSelf->__PVT__reg_op2);
    vlSelf->__PVT__is_lbu_lhu_lw = ((IData)(vlSelf->__PVT__instr_lbu) 
                                    | ((IData)(vlSelf->__PVT__instr_lhu) 
                                       | (IData)(vlSelf->__PVT__instr_lw)));
    vlSelf->__PVT__cpuregs_wrdata = 0U;
    if ((0x40U == (IData)(vlSelf->__PVT__cpu_state))) {
        if (vlSelf->__PVT__latched_branch) {
            vlSelf->__PVT__cpuregs_write = 1U;
            vlSelf->__PVT__cpuregs_wrdata = (vlSelf->__PVT__reg_pc 
                                             + ((IData)(vlSelf->__PVT__latched_compr)
                                                 ? 2U
                                                 : 4U));
        } else if (((IData)(vlSelf->__PVT__latched_store) 
                    & (~ (IData)(vlSelf->__PVT__latched_branch)))) {
            vlSelf->__PVT__cpuregs_write = 1U;
            vlSelf->__PVT__cpuregs_wrdata = ((IData)(vlSelf->__PVT__latched_stalu)
                                              ? vlSelf->__PVT__alu_out_q
                                              : vlSelf->__PVT__reg_out);
        }
    }
    if (((IData)(vlSelf->__PVT__mem_do_rinst) & (IData)(vlSelf->__PVT__mem_done))) {
        vlSelf->__PVT__decoded_rd = (0x1fU & (vlSelf->__PVT__mem_rdata_latched 
                                              >> 7U));
        vlSelf->__PVT__decoded_rs2 = (0x1fU & (vlSelf->__PVT__mem_rdata_latched 
                                               >> 0x14U));
    }
    vlSelf->__PVT__cpuregs_rs2 = ((0U != (IData)(vlSelf->__PVT__decoded_rs2))
                                   ? vlSelf->__PVT__cpuregs
                                  [vlSelf->__PVT__decoded_rs2]
                                   : 0U);
    if (((IData)(vlSelf->__PVT__decoder_trigger) & 
         (~ (IData)(vlSelf->__PVT__decoder_pseudo_trigger)))) {
        vlSelf->__PVT__instr_rdcycle = ((IData)((0xc0002073U 
                                                 == 
                                                 (0xfffff07fU 
                                                  & vlSelf->__PVT__mem_rdata_q))) 
                                        | (IData)((0xc0102073U 
                                                   == 
                                                   (0xfffff07fU 
                                                    & vlSelf->__PVT__mem_rdata_q))));
        vlSelf->__PVT__instr_rdcycleh = ((IData)((0xc8002073U 
                                                  == 
                                                  (0xfffff07fU 
                                                   & vlSelf->__PVT__mem_rdata_q))) 
                                         | (IData)(
                                                   (0xc8102073U 
                                                    == 
                                                    (0xfffff07fU 
                                                     & vlSelf->__PVT__mem_rdata_q))));
        vlSelf->__PVT__instr_rdinstr = (IData)((0xc0202073U 
                                                == 
                                                (0xfffff07fU 
                                                 & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_rdinstrh = (IData)((0xc8202073U 
                                                 == 
                                                 (0xfffff07fU 
                                                  & vlSelf->__PVT__mem_rdata_q)));
    }
    vlSelf->__PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh 
        = ((IData)(vlSelf->__PVT__instr_rdcycle) | 
           ((IData)(vlSelf->__PVT__instr_rdcycleh) 
            | ((IData)(vlSelf->__PVT__instr_rdinstr) 
               | (IData)(vlSelf->__PVT__instr_rdinstrh))));
    vlSelf->__PVT__is_lui_auipc_jal = ((IData)(vlSelf->__PVT__instr_lui) 
                                       | ((IData)(vlSelf->__PVT__instr_auipc) 
                                          | (IData)(vlSelf->__PVT__instr_jal)));
    vlSelf->__PVT__is_lui_auipc_jal_jalr_addi_add_sub 
        = ((IData)(vlSelf->__PVT__instr_lui) | ((IData)(vlSelf->__PVT__instr_auipc) 
                                                | ((IData)(vlSelf->__PVT__instr_jal) 
                                                   | ((IData)(vlSelf->__PVT__instr_jalr) 
                                                      | ((IData)(vlSelf->__PVT__instr_addi) 
                                                         | ((IData)(vlSelf->__PVT__instr_add) 
                                                            | (IData)(vlSelf->__PVT__instr_sub)))))));
    vlSelf->__PVT__is_slti_blt_slt = ((IData)(vlSelf->__PVT__instr_slti) 
                                      | ((IData)(vlSelf->__PVT__instr_blt) 
                                         | (IData)(vlSelf->__PVT__instr_slt)));
    vlSelf->__PVT__is_sltiu_bltu_sltu = ((IData)(vlSelf->__PVT__instr_sltiu) 
                                         | ((IData)(vlSelf->__PVT__instr_bltu) 
                                            | (IData)(vlSelf->__PVT__instr_sltu)));
    vlSelf->__PVT__is_compare = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                 | ((IData)(vlSelf->__PVT__instr_slti) 
                                    | ((IData)(vlSelf->__PVT__instr_slt) 
                                       | ((IData)(vlSelf->__PVT__instr_sltiu) 
                                          | (IData)(vlSelf->__PVT__instr_sltu)))));
    if (((IData)(vlSelf->__PVT__decoder_trigger) & 
         (~ (IData)(vlSelf->__PVT__decoder_pseudo_trigger)))) {
        vlSelf->__PVT__instr_beq = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                    & (0U == (0x7000U 
                                              & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_bne = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                    & (0x1000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_blt = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                    & (0x4000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_bge = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                    & (0x5000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_bltu = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                     & (0x6000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_bgeu = ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu) 
                                     & (0x7000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lb = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                   & (0U == (0x7000U 
                                             & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lh = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                   & (0x1000U == (0x7000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lw = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                   & (0x2000U == (0x7000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lbu = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                    & (0x4000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_lhu = ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                    & (0x5000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sb = ((IData)(vlSelf->__PVT__is_sb_sh_sw) 
                                   & (0U == (0x7000U 
                                             & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sh = ((IData)(vlSelf->__PVT__is_sb_sh_sw) 
                                   & (0x1000U == (0x7000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sw = ((IData)(vlSelf->__PVT__is_sb_sh_sw) 
                                   & (0x2000U == (0x7000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_addi = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0U == (0x7000U 
                                               & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_slti = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x2000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sltiu = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                      & (0x3000U == 
                                         (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_xori = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x4000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_ori = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                    & (0x6000U == (0x7000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_andi = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x7000U == 
                                        (0x7000U & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_slli = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x1000U == 
                                        (0xfe007000U 
                                         & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_srli = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x5000U == 
                                        (0xfe007000U 
                                         & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_srai = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                     & (0x40005000U 
                                        == (0xfe007000U 
                                            & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__is_slli_srli_srai = ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                                            & ((IData)(
                                                       (0x1000U 
                                                        == 
                                                        (0xfe007000U 
                                                         & vlSelf->__PVT__mem_rdata_q))) 
                                               | ((IData)(
                                                          (0x5000U 
                                                           == 
                                                           (0xfe007000U 
                                                            & vlSelf->__PVT__mem_rdata_q))) 
                                                  | (IData)(
                                                            (0x40005000U 
                                                             == 
                                                             (0xfe007000U 
                                                              & vlSelf->__PVT__mem_rdata_q))))));
        vlSelf->__PVT__is_jalr_addi_slti_sltiu_xori_ori_andi 
            = ((IData)(vlSelf->__PVT__instr_jalr) | 
               ((IData)(vlSelf->__PVT__is_alu_reg_imm) 
                & ((0U == (7U & (vlSelf->__PVT__mem_rdata_q 
                                 >> 0xcU))) | ((2U 
                                                == 
                                                (7U 
                                                 & (vlSelf->__PVT__mem_rdata_q 
                                                    >> 0xcU))) 
                                               | ((3U 
                                                   == 
                                                   (7U 
                                                    & (vlSelf->__PVT__mem_rdata_q 
                                                       >> 0xcU))) 
                                                  | ((4U 
                                                      == 
                                                      (7U 
                                                       & (vlSelf->__PVT__mem_rdata_q 
                                                          >> 0xcU))) 
                                                     | ((6U 
                                                         == 
                                                         (7U 
                                                          & (vlSelf->__PVT__mem_rdata_q 
                                                             >> 0xcU))) 
                                                        | (7U 
                                                           == 
                                                           (7U 
                                                            & (vlSelf->__PVT__mem_rdata_q 
                                                               >> 0xcU))))))))));
        vlSelf->__PVT__is_lui_auipc_jal_jalr_addi_add_sub = 0U;
        vlSelf->__PVT__is_compare = 0U;
        vlSelf->__PVT__decoded_imm = ((IData)(vlSelf->__PVT__instr_jal)
                                       ? vlSelf->__PVT__decoded_imm_j
                                       : (((IData)(vlSelf->__PVT__instr_lui) 
                                           | (IData)(vlSelf->__PVT__instr_auipc))
                                           ? VL_SHIFTL_III(32,32,32, 
                                                           (vlSelf->__PVT__mem_rdata_q 
                                                            >> 0xcU), 0xcU)
                                           : (((IData)(vlSelf->__PVT__instr_jalr) 
                                               | ((IData)(vlSelf->__PVT__is_lb_lh_lw_lbu_lhu) 
                                                  | (IData)(vlSelf->__PVT__is_alu_reg_imm)))
                                               ? VL_EXTENDS_II(32,12, 
                                                               (vlSelf->__PVT__mem_rdata_q 
                                                                >> 0x14U))
                                               : ((IData)(vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu)
                                                   ? 
                                                  VL_EXTENDS_II(32,13, 
                                                                ((0x1000U 
                                                                  & (vlSelf->__PVT__mem_rdata_q 
                                                                     >> 0x13U)) 
                                                                 | ((0x800U 
                                                                     & (vlSelf->__PVT__mem_rdata_q 
                                                                        << 4U)) 
                                                                    | ((0x7e0U 
                                                                        & (vlSelf->__PVT__mem_rdata_q 
                                                                           >> 0x14U)) 
                                                                       | (0x1eU 
                                                                          & (vlSelf->__PVT__mem_rdata_q 
                                                                             >> 7U))))))
                                                   : 
                                                  ((IData)(vlSelf->__PVT__is_sb_sh_sw)
                                                    ? 
                                                   VL_EXTENDS_II(32,12, 
                                                                 ((0xfe0U 
                                                                   & (vlSelf->__PVT__mem_rdata_q 
                                                                      >> 0x14U)) 
                                                                  | (0x1fU 
                                                                     & (vlSelf->__PVT__mem_rdata_q 
                                                                        >> 7U))))
                                                    : 0U)))));
        vlSelf->__PVT__instr_add = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0U == (0xfe007000U 
                                              & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sub = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x40000000U 
                                       == (0xfe007000U 
                                           & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sll = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x1000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_slt = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x2000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sltu = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                     & (0x3000U == 
                                        (0xfe007000U 
                                         & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_xor = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x4000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_srl = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x5000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_sra = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x40005000U 
                                       == (0xfe007000U 
                                           & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_or = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                   & (0x6000U == (0xfe007000U 
                                                  & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__instr_and = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                    & (0x7000U == (0xfe007000U 
                                                   & vlSelf->__PVT__mem_rdata_q)));
        vlSelf->__PVT__is_sll_srl_sra = ((IData)(vlSelf->__PVT__is_alu_reg_reg) 
                                         & ((IData)(
                                                    (0x1000U 
                                                     == 
                                                     (0xfe007000U 
                                                      & vlSelf->__PVT__mem_rdata_q))) 
                                            | ((IData)(
                                                       (0x5000U 
                                                        == 
                                                        (0xfe007000U 
                                                         & vlSelf->__PVT__mem_rdata_q))) 
                                               | (IData)(
                                                         (0x40005000U 
                                                          == 
                                                          (0xfe007000U 
                                                           & vlSelf->__PVT__mem_rdata_q))))));
    }
    if (((IData)(vlSelf->__PVT__mem_do_rinst) & (IData)(vlSelf->__PVT__mem_done))) {
        vlSelf->__PVT__compressed_instr = 0U;
        vlSelf->__PVT__is_alu_reg_imm = (0x13U == (0x7fU 
                                                   & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__is_lb_lh_lw_lbu_lhu = (3U == 
                                              (0x7fU 
                                               & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__is_sb_sh_sw = (0x23U == (0x7fU 
                                                & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__decoded_imm_j = ((0xfffffU & vlSelf->__PVT__decoded_imm_j) 
                                        | (0xfff00000U 
                                           & VL_EXTENDS_II(32,21, 
                                                           (0x1ffffeU 
                                                            & (vlSelf->__PVT__mem_rdata_latched 
                                                               >> 0xbU)))));
        vlSelf->__PVT__decoded_imm_j = ((0xfffff801U 
                                         & vlSelf->__PVT__decoded_imm_j) 
                                        | (0x7feU & 
                                           (VL_EXTENDS_II(32,21, 
                                                          (0x1ffffeU 
                                                           & (vlSelf->__PVT__mem_rdata_latched 
                                                              >> 0xbU))) 
                                            >> 9U)));
        vlSelf->__PVT__decoded_imm_j = ((0xfffff7ffU 
                                         & vlSelf->__PVT__decoded_imm_j) 
                                        | (0x800U & 
                                           (VL_EXTENDS_II(32,21, 
                                                          (0x1ffffeU 
                                                           & (vlSelf->__PVT__mem_rdata_latched 
                                                              >> 0xbU))) 
                                            << 2U)));
        vlSelf->__PVT__decoded_imm_j = ((0xfff00fffU 
                                         & vlSelf->__PVT__decoded_imm_j) 
                                        | (0xff000U 
                                           & (VL_EXTENDS_II(32,21, 
                                                            (0x1ffffeU 
                                                             & (vlSelf->__PVT__mem_rdata_latched 
                                                                >> 0xbU))) 
                                              << 0xbU)));
        vlSelf->__PVT__decoded_imm_j = ((0xfffffffeU 
                                         & vlSelf->__PVT__decoded_imm_j) 
                                        | (1U & VL_EXTENDS_II(1,21, 
                                                              (0x1ffffeU 
                                                               & (vlSelf->__PVT__mem_rdata_latched 
                                                                  >> 0xbU)))));
        vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu 
            = (0x63U == (0x7fU & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__instr_auipc = (0x17U == (0x7fU 
                                                & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__instr_lui = (0x37U == (0x7fU 
                                              & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__instr_jal = (0x6fU == (0x7fU 
                                              & vlSelf->__PVT__mem_rdata_latched));
        vlSelf->__PVT__instr_jalr = (IData)((0x67U 
                                             == (0x707fU 
                                                 & vlSelf->__PVT__mem_rdata_latched)));
        vlSelf->__PVT__is_alu_reg_reg = (0x33U == (0x7fU 
                                                   & vlSelf->__PVT__mem_rdata_latched));
    }
    if (vlSymsp->TOP.wb_rst_i) {
        vlSelf->__PVT__is_compare = 0U;
        vlSelf->__PVT__instr_beq = 0U;
        vlSelf->__PVT__instr_bne = 0U;
        vlSelf->__PVT__instr_blt = 0U;
        vlSelf->__PVT__instr_bge = 0U;
        vlSelf->__PVT__instr_bltu = 0U;
        vlSelf->__PVT__instr_bgeu = 0U;
        vlSelf->__PVT__instr_addi = 0U;
        vlSelf->__PVT__instr_slti = 0U;
        vlSelf->__PVT__instr_sltiu = 0U;
        vlSelf->__PVT__instr_xori = 0U;
        vlSelf->__PVT__instr_ori = 0U;
        vlSelf->__PVT__instr_andi = 0U;
        vlSelf->__PVT__is_beq_bne_blt_bge_bltu_bgeu = 0U;
        vlSelf->__PVT__instr_add = 0U;
        vlSelf->__PVT__instr_sub = 0U;
        vlSelf->__PVT__instr_sll = 0U;
        vlSelf->__PVT__instr_slt = 0U;
        vlSelf->__PVT__instr_sltu = 0U;
        vlSelf->__PVT__instr_xor = 0U;
        vlSelf->__PVT__instr_srl = 0U;
        vlSelf->__PVT__instr_sra = 0U;
        vlSelf->__PVT__instr_or = 0U;
        vlSelf->__PVT__instr_and = 0U;
    }
    vlSelf->__PVT__decoder_pseudo_trigger = __Vdly__decoder_pseudo_trigger;
    vlSelf->__PVT__decoder_trigger = __Vdly__decoder_trigger;
    if (vlSelf->__PVT__mem_xfer) {
        vlSelf->__PVT__mem_rdata_q = vlSymsp->TOP.picorv32_wb__DOT__mem_rdata;
    }
    vlSelf->__PVT__alu_out_0 = 0U;
    if (vlSelf->__PVT__instr_beq) {
        vlSelf->__PVT__alu_out_0 = __PVT__alu_eq;
    } else if (vlSelf->__PVT__instr_bne) {
        vlSelf->__PVT__alu_out_0 = (1U & (~ (IData)(__PVT__alu_eq)));
    } else if (vlSelf->__PVT__instr_bge) {
        vlSelf->__PVT__alu_out_0 = (1U & (~ (IData)(__PVT__alu_lts)));
    } else if (vlSelf->__PVT__instr_bgeu) {
        vlSelf->__PVT__alu_out_0 = (1U & (~ (IData)(__PVT__alu_ltu)));
    } else if (vlSelf->__PVT__is_slti_blt_slt) {
        vlSelf->__PVT__alu_out_0 = __PVT__alu_lts;
    } else if (vlSelf->__PVT__is_sltiu_bltu_sltu) {
        vlSelf->__PVT__alu_out_0 = __PVT__alu_ltu;
    }
    vlSelf->__PVT__alu_out = 0U;
    if (vlSelf->__PVT__is_lui_auipc_jal_jalr_addi_add_sub) {
        vlSelf->__PVT__alu_out = ((IData)(vlSelf->__PVT__instr_sub)
                                   ? (vlSelf->__PVT__reg_op1 
                                      - vlSelf->__PVT__reg_op2)
                                   : (vlSelf->__PVT__reg_op1 
                                      + vlSelf->__PVT__reg_op2));
    } else if (vlSelf->__PVT__is_compare) {
        vlSelf->__PVT__alu_out = vlSelf->__PVT__alu_out_0;
    } else if (((IData)(vlSelf->__PVT__instr_xori) 
                | (IData)(vlSelf->__PVT__instr_xor))) {
        vlSelf->__PVT__alu_out = (vlSelf->__PVT__reg_op1 
                                  ^ vlSelf->__PVT__reg_op2);
    } else if (((IData)(vlSelf->__PVT__instr_ori) | (IData)(vlSelf->__PVT__instr_or))) {
        vlSelf->__PVT__alu_out = (vlSelf->__PVT__reg_op1 
                                  | vlSelf->__PVT__reg_op2);
    } else if (((IData)(vlSelf->__PVT__instr_andi) 
                | (IData)(vlSelf->__PVT__instr_and))) {
        vlSelf->__PVT__alu_out = (vlSelf->__PVT__reg_op1 
                                  & vlSelf->__PVT__reg_op2);
    }
    vlSelf->__PVT__instr_trap = (1U & (~ ((IData)(vlSelf->__PVT__instr_lui) 
                                          | ((IData)(vlSelf->__PVT__instr_auipc) 
                                             | ((IData)(vlSelf->__PVT__instr_jal) 
                                                | ((IData)(vlSelf->__PVT__instr_jalr) 
                                                   | ((IData)(vlSelf->__PVT__instr_beq) 
                                                      | ((IData)(vlSelf->__PVT__instr_bne) 
                                                         | ((IData)(vlSelf->__PVT__instr_blt) 
                                                            | ((IData)(vlSelf->__PVT__instr_bge) 
                                                               | ((IData)(vlSelf->__PVT__instr_bltu) 
                                                                  | ((IData)(vlSelf->__PVT__instr_bgeu) 
                                                                     | ((IData)(vlSelf->__PVT__instr_lb) 
                                                                        | ((IData)(vlSelf->__PVT__instr_lh) 
                                                                           | ((IData)(vlSelf->__PVT__instr_lw) 
                                                                              | ((IData)(vlSelf->__PVT__instr_lbu) 
                                                                                | ((IData)(vlSelf->__PVT__instr_lhu) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sb) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sh) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sw) 
                                                                                | ((IData)(vlSelf->__PVT__instr_addi) 
                                                                                | ((IData)(vlSelf->__PVT__instr_slti) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sltiu) 
                                                                                | ((IData)(vlSelf->__PVT__instr_xori) 
                                                                                | ((IData)(vlSelf->__PVT__instr_ori) 
                                                                                | ((IData)(vlSelf->__PVT__instr_andi) 
                                                                                | ((IData)(vlSelf->__PVT__instr_slli) 
                                                                                | ((IData)(vlSelf->__PVT__instr_srli) 
                                                                                | ((IData)(vlSelf->__PVT__instr_srai) 
                                                                                | ((IData)(vlSelf->__PVT__instr_add) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sub) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sll) 
                                                                                | ((IData)(vlSelf->__PVT__instr_slt) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sltu) 
                                                                                | ((IData)(vlSelf->__PVT__instr_xor) 
                                                                                | ((IData)(vlSelf->__PVT__instr_srl) 
                                                                                | ((IData)(vlSelf->__PVT__instr_sra) 
                                                                                | ((IData)(vlSelf->__PVT__instr_or) 
                                                                                | ((IData)(vlSelf->__PVT__instr_and) 
                                                                                | ((IData)(vlSelf->__PVT__instr_rdcycle) 
                                                                                | ((IData)(vlSelf->__PVT__instr_rdcycleh) 
                                                                                | ((IData)(vlSelf->__PVT__instr_rdinstr) 
                                                                                | ((IData)(vlSelf->__PVT__instr_rdinstrh) 
                                                                                | ((IData)(vlSelf->__PVT__instr_fence) 
                                                                                | ((IData)(vlSelf->__PVT__instr_getq) 
                                                                                | ((IData)(vlSelf->__PVT__instr_setq) 
                                                                                | ((IData)(vlSelf->__PVT__compressed_instr) 
                                                                                | ((IData)(vlSelf->__PVT__instr_maskirq) 
                                                                                | ((IData)(vlSelf->__PVT__instr_timer) 
                                                                                | (IData)(vlSelf->__PVT__instr_waitirq))))))))))))))))))))))))))))))))))))))))))))))))));
    vlSelf->__PVT__mem_do_rinst = vlSelf->__Vdly__mem_do_rinst;
    vlSelf->__PVT__mem_la_read = ((~ (IData)(vlSymsp->TOP.wb_rst_i)) 
                                  & ((~ (IData)((0U 
                                                 != (IData)(vlSelf->__PVT__mem_state)))) 
                                     & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                        | ((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                           | (IData)(vlSelf->__PVT__mem_do_rdata)))));
}

VL_INLINE_OPT void Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__2(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___nba_sequent__TOP__picorv32_wb__DOT__picorv32_core__2\n"); );
    // Body
    vlSelf->__PVT__mem_xfer = ((IData)(vlSymsp->TOP.picorv32_wb__DOT__mem_ready) 
                               & (IData)(vlSelf->mem_valid));
    if ((0U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_rdata_word = vlSymsp->TOP.picorv32_wb__DOT__mem_rdata;
    } else if ((1U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        if ((2U & vlSelf->__PVT__reg_op1)) {
            if ((2U & vlSelf->__PVT__reg_op1)) {
                vlSelf->__PVT__mem_rdata_word = (vlSymsp->TOP.picorv32_wb__DOT__mem_rdata 
                                                 >> 0x10U);
            }
        } else {
            vlSelf->__PVT__mem_rdata_word = (0xffffU 
                                             & vlSymsp->TOP.picorv32_wb__DOT__mem_rdata);
        }
    } else if ((2U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_rdata_word = ((2U & vlSelf->__PVT__reg_op1)
                                          ? ((1U & vlSelf->__PVT__reg_op1)
                                              ? (vlSymsp->TOP.picorv32_wb__DOT__mem_rdata 
                                                 >> 0x18U)
                                              : (0xffU 
                                                 & (vlSymsp->TOP.picorv32_wb__DOT__mem_rdata 
                                                    >> 0x10U)))
                                          : ((1U & vlSelf->__PVT__reg_op1)
                                              ? (0xffU 
                                                 & (vlSymsp->TOP.picorv32_wb__DOT__mem_rdata 
                                                    >> 8U))
                                              : (0xffU 
                                                 & vlSymsp->TOP.picorv32_wb__DOT__mem_rdata)));
    }
    vlSelf->__PVT__mem_rdata_latched = ((IData)(vlSelf->__PVT__mem_xfer)
                                         ? vlSymsp->TOP.picorv32_wb__DOT__mem_rdata
                                         : vlSelf->__PVT__mem_rdata_q);
    vlSelf->__PVT__mem_done = ((~ (IData)(vlSymsp->TOP.wb_rst_i)) 
                               & (((IData)(vlSelf->__PVT__mem_xfer) 
                                   & ((0U != (IData)(vlSelf->__PVT__mem_state)) 
                                      & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                         | ((IData)(vlSelf->__PVT__mem_do_rdata) 
                                            | (IData)(vlSelf->__PVT__mem_do_wdata))))) 
                                  | ((3U == (IData)(vlSelf->__PVT__mem_state)) 
                                     & (IData)(vlSelf->__PVT__mem_do_rinst))));
}
