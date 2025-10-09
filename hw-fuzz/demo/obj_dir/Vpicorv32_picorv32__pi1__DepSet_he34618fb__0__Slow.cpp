// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vpicorv32.h for the primary calling header

#include "Vpicorv32__pch.h"
#include "Vpicorv32__Syms.h"
#include "Vpicorv32_picorv32__pi1.h"

VL_ATTR_COLD void Vpicorv32_picorv32__pi1___stl_sequent__TOP__picorv32_axi__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___stl_sequent__TOP__picorv32_axi__DOT__picorv32_core__0\n"); );
    // Init
    CData/*0:0*/ __PVT__alu_eq;
    __PVT__alu_eq = 0;
    CData/*0:0*/ __PVT__alu_ltu;
    __PVT__alu_ltu = 0;
    CData/*0:0*/ __PVT__alu_lts;
    __PVT__alu_lts = 0;
    // Body
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
    vlSelf->__PVT__cpuregs_rs1 = ((0U != (IData)(vlSelf->__PVT__decoded_rs1))
                                   ? vlSelf->__PVT__cpuregs
                                  [vlSelf->__PVT__decoded_rs1]
                                   : 0U);
    vlSelf->__PVT__cpuregs_rs2 = ((0U != (IData)(vlSelf->__PVT__decoded_rs2))
                                   ? vlSelf->__PVT__cpuregs
                                  [vlSelf->__PVT__decoded_rs2]
                                   : 0U);
    vlSelf->__PVT__cpuregs_write = 0U;
    vlSelf->__PVT__mem_la_write = ((IData)(vlSymsp->TOP.resetn) 
                                   & ((~ (IData)((0U 
                                                  != (IData)(vlSelf->__PVT__mem_state)))) 
                                      & (IData)(vlSelf->__PVT__mem_do_wdata)));
    vlSelf->__PVT__next_pc = (((IData)(vlSelf->__PVT__latched_branch) 
                               & (IData)(vlSelf->__PVT__latched_store))
                               ? (0xfffffffeU & vlSelf->__PVT__reg_out)
                               : vlSelf->__PVT__reg_next_pc);
    vlSelf->__PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh 
        = ((IData)(vlSelf->__PVT__instr_rdcycle) | 
           ((IData)(vlSelf->__PVT__instr_rdcycleh) 
            | ((IData)(vlSelf->__PVT__instr_rdinstr) 
               | (IData)(vlSelf->__PVT__instr_rdinstrh))));
    vlSelf->__PVT__mem_la_read = ((IData)(vlSymsp->TOP.resetn) 
                                  & ((~ (IData)((0U 
                                                 != (IData)(vlSelf->__PVT__mem_state)))) 
                                     & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                        | ((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                           | (IData)(vlSelf->__PVT__mem_do_rdata)))));
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
    __PVT__alu_eq = (vlSelf->__PVT__reg_op1 == vlSelf->__PVT__reg_op2);
    __PVT__alu_lts = VL_LTS_III(32, vlSelf->__PVT__reg_op1, vlSelf->__PVT__reg_op2);
    __PVT__alu_ltu = (vlSelf->__PVT__reg_op1 < vlSelf->__PVT__reg_op2);
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
    vlSelf->__PVT__mem_xfer = ((IData)(vlSymsp->TOP.picorv32_axi__DOT__mem_ready) 
                               & (IData)(vlSelf->mem_valid));
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

VL_ATTR_COLD void Vpicorv32_picorv32__pi1___stl_sequent__TOP__picorv32_wb__DOT__picorv32_core__0(Vpicorv32_picorv32__pi1* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vpicorv32__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+        Vpicorv32_picorv32__pi1___stl_sequent__TOP__picorv32_wb__DOT__picorv32_core__0\n"); );
    // Init
    CData/*0:0*/ __PVT__alu_eq;
    __PVT__alu_eq = 0;
    CData/*0:0*/ __PVT__alu_ltu;
    __PVT__alu_ltu = 0;
    CData/*0:0*/ __PVT__alu_lts;
    __PVT__alu_lts = 0;
    // Body
    if ((0U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_la_wdata = vlSelf->__PVT__reg_op2;
        vlSelf->__PVT__mem_la_wstrb = 0xfU;
        vlSelf->__PVT__mem_rdata_word = vlSymsp->TOP.picorv32_wb__DOT__mem_rdata;
    } else if ((1U == (IData)(vlSelf->__PVT__mem_wordsize))) {
        vlSelf->__PVT__mem_la_wdata = ((vlSelf->__PVT__reg_op2 
                                        << 0x10U) | 
                                       (0xffffU & vlSelf->__PVT__reg_op2));
        if ((2U & vlSelf->__PVT__reg_op1)) {
            vlSelf->__PVT__mem_la_wstrb = 0xcU;
            if ((2U & vlSelf->__PVT__reg_op1)) {
                vlSelf->__PVT__mem_rdata_word = (vlSymsp->TOP.picorv32_wb__DOT__mem_rdata 
                                                 >> 0x10U);
            }
        } else {
            vlSelf->__PVT__mem_la_wstrb = 3U;
            vlSelf->__PVT__mem_rdata_word = (0xffffU 
                                             & vlSymsp->TOP.picorv32_wb__DOT__mem_rdata);
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
    vlSelf->__PVT__cpuregs_rs1 = ((0U != (IData)(vlSelf->__PVT__decoded_rs1))
                                   ? vlSelf->__PVT__cpuregs
                                  [vlSelf->__PVT__decoded_rs1]
                                   : 0U);
    vlSelf->__PVT__cpuregs_rs2 = ((0U != (IData)(vlSelf->__PVT__decoded_rs2))
                                   ? vlSelf->__PVT__cpuregs
                                  [vlSelf->__PVT__decoded_rs2]
                                   : 0U);
    vlSelf->__PVT__cpuregs_write = 0U;
    vlSelf->__PVT__mem_la_write = ((~ (IData)(vlSymsp->TOP.wb_rst_i)) 
                                   & ((~ (IData)((0U 
                                                  != (IData)(vlSelf->__PVT__mem_state)))) 
                                      & (IData)(vlSelf->__PVT__mem_do_wdata)));
    vlSelf->__PVT__next_pc = (((IData)(vlSelf->__PVT__latched_branch) 
                               & (IData)(vlSelf->__PVT__latched_store))
                               ? (0xfffffffeU & vlSelf->__PVT__reg_out)
                               : vlSelf->__PVT__reg_next_pc);
    vlSelf->__PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh 
        = ((IData)(vlSelf->__PVT__instr_rdcycle) | 
           ((IData)(vlSelf->__PVT__instr_rdcycleh) 
            | ((IData)(vlSelf->__PVT__instr_rdinstr) 
               | (IData)(vlSelf->__PVT__instr_rdinstrh))));
    vlSelf->__PVT__mem_la_read = ((~ (IData)(vlSymsp->TOP.wb_rst_i)) 
                                  & ((~ (IData)((0U 
                                                 != (IData)(vlSelf->__PVT__mem_state)))) 
                                     & ((IData)(vlSelf->__PVT__mem_do_rinst) 
                                        | ((IData)(vlSelf->__PVT__mem_do_prefetch) 
                                           | (IData)(vlSelf->__PVT__mem_do_rdata)))));
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
    vlSelf->__PVT__mem_xfer = ((IData)(vlSymsp->TOP.picorv32_wb__DOT__mem_ready) 
                               & (IData)(vlSelf->mem_valid));
    __PVT__alu_eq = (vlSelf->__PVT__reg_op1 == vlSelf->__PVT__reg_op2);
    __PVT__alu_lts = VL_LTS_III(32, vlSelf->__PVT__reg_op1, vlSelf->__PVT__reg_op2);
    __PVT__alu_ltu = (vlSelf->__PVT__reg_op1 < vlSelf->__PVT__reg_op2);
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
}
