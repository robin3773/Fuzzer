// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vpicorv32.h for the primary calling header

#ifndef VERILATED_VPICORV32_PICORV32__PI1_H_
#define VERILATED_VPICORV32_PICORV32__PI1_H_  // guard

#include "verilated.h"


class Vpicorv32__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vpicorv32_picorv32__pi1 final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    // Anonymous structures to workaround compiler member-count bugs
    struct {
        VL_IN8(clk,0,0);
        VL_IN8(resetn,0,0);
        VL_OUT8(trap,0,0);
        VL_OUT8(mem_valid,0,0);
        VL_OUT8(mem_instr,0,0);
        VL_IN8(mem_ready,0,0);
        VL_OUT8(mem_wstrb,3,0);
        VL_OUT8(__PVT__mem_la_read,0,0);
        VL_OUT8(__PVT__mem_la_write,0,0);
        VL_OUT8(__PVT__mem_la_wstrb,3,0);
        VL_OUT8(pcpi_valid,0,0);
        VL_IN8(pcpi_wr,0,0);
        VL_IN8(pcpi_wait,0,0);
        VL_IN8(pcpi_ready,0,0);
        VL_OUT8(trace_valid,0,0);
        CData/*4:0*/ __PVT__reg_sh;
        CData/*0:0*/ __PVT__irq_active;
        CData/*1:0*/ __PVT__mem_state;
        CData/*1:0*/ __PVT__mem_wordsize;
        CData/*0:0*/ __PVT__mem_do_prefetch;
        CData/*0:0*/ __PVT__mem_do_rinst;
        CData/*0:0*/ __PVT__mem_do_rdata;
        CData/*0:0*/ __PVT__mem_do_wdata;
        CData/*0:0*/ __PVT__mem_xfer;
        CData/*0:0*/ __PVT__mem_done;
        CData/*0:0*/ __PVT__instr_lui;
        CData/*0:0*/ __PVT__instr_auipc;
        CData/*0:0*/ __PVT__instr_jal;
        CData/*0:0*/ __PVT__instr_jalr;
        CData/*0:0*/ __PVT__instr_beq;
        CData/*0:0*/ __PVT__instr_bne;
        CData/*0:0*/ __PVT__instr_blt;
        CData/*0:0*/ __PVT__instr_bge;
        CData/*0:0*/ __PVT__instr_bltu;
        CData/*0:0*/ __PVT__instr_bgeu;
        CData/*0:0*/ __PVT__instr_lb;
        CData/*0:0*/ __PVT__instr_lh;
        CData/*0:0*/ __PVT__instr_lw;
        CData/*0:0*/ __PVT__instr_lbu;
        CData/*0:0*/ __PVT__instr_lhu;
        CData/*0:0*/ __PVT__instr_sb;
        CData/*0:0*/ __PVT__instr_sh;
        CData/*0:0*/ __PVT__instr_sw;
        CData/*0:0*/ __PVT__instr_addi;
        CData/*0:0*/ __PVT__instr_slti;
        CData/*0:0*/ __PVT__instr_sltiu;
        CData/*0:0*/ __PVT__instr_xori;
        CData/*0:0*/ __PVT__instr_ori;
        CData/*0:0*/ __PVT__instr_andi;
        CData/*0:0*/ __PVT__instr_slli;
        CData/*0:0*/ __PVT__instr_srli;
        CData/*0:0*/ __PVT__instr_srai;
        CData/*0:0*/ __PVT__instr_add;
        CData/*0:0*/ __PVT__instr_sub;
        CData/*0:0*/ __PVT__instr_sll;
        CData/*0:0*/ __PVT__instr_slt;
        CData/*0:0*/ __PVT__instr_sltu;
        CData/*0:0*/ __PVT__instr_xor;
        CData/*0:0*/ __PVT__instr_srl;
        CData/*0:0*/ __PVT__instr_sra;
        CData/*0:0*/ __PVT__instr_or;
        CData/*0:0*/ __PVT__instr_and;
        CData/*0:0*/ __PVT__instr_rdcycle;
        CData/*0:0*/ __PVT__instr_rdcycleh;
    };
    struct {
        CData/*0:0*/ __PVT__instr_rdinstr;
        CData/*0:0*/ __PVT__instr_rdinstrh;
        CData/*0:0*/ __PVT__instr_fence;
        CData/*0:0*/ __PVT__instr_getq;
        CData/*0:0*/ __PVT__instr_setq;
        CData/*0:0*/ __PVT__instr_maskirq;
        CData/*0:0*/ __PVT__instr_waitirq;
        CData/*0:0*/ __PVT__instr_timer;
        CData/*0:0*/ __PVT__instr_trap;
        CData/*4:0*/ __PVT__decoded_rd;
        CData/*4:0*/ __PVT__decoded_rs1;
        CData/*4:0*/ __PVT__decoded_rs2;
        CData/*0:0*/ __PVT__decoder_trigger;
        CData/*0:0*/ __PVT__decoder_pseudo_trigger;
        CData/*0:0*/ __PVT__compressed_instr;
        CData/*0:0*/ __PVT__is_lui_auipc_jal;
        CData/*0:0*/ __PVT__is_lb_lh_lw_lbu_lhu;
        CData/*0:0*/ __PVT__is_slli_srli_srai;
        CData/*0:0*/ __PVT__is_jalr_addi_slti_sltiu_xori_ori_andi;
        CData/*0:0*/ __PVT__is_sb_sh_sw;
        CData/*0:0*/ __PVT__is_sll_srl_sra;
        CData/*0:0*/ __PVT__is_lui_auipc_jal_jalr_addi_add_sub;
        CData/*0:0*/ __PVT__is_slti_blt_slt;
        CData/*0:0*/ __PVT__is_sltiu_bltu_sltu;
        CData/*0:0*/ __PVT__is_beq_bne_blt_bge_bltu_bgeu;
        CData/*0:0*/ __PVT__is_lbu_lhu_lw;
        CData/*0:0*/ __PVT__is_alu_reg_imm;
        CData/*0:0*/ __PVT__is_alu_reg_reg;
        CData/*0:0*/ __PVT__is_compare;
        CData/*0:0*/ __PVT__is_rdcycle_rdcycleh_rdinstr_rdinstrh;
        CData/*7:0*/ __PVT__cpu_state;
        CData/*0:0*/ __PVT__latched_store;
        CData/*0:0*/ __PVT__latched_stalu;
        CData/*0:0*/ __PVT__latched_branch;
        CData/*0:0*/ __PVT__latched_compr;
        CData/*0:0*/ __PVT__latched_is_lu;
        CData/*0:0*/ __PVT__latched_is_lh;
        CData/*0:0*/ __PVT__latched_is_lb;
        CData/*4:0*/ __PVT__latched_rd;
        CData/*0:0*/ __PVT__do_waitirq;
        CData/*0:0*/ __PVT__alu_out_0;
        CData/*0:0*/ __PVT__cpuregs_write;
        CData/*1:0*/ __Vdly__mem_state;
        CData/*0:0*/ __Vdly__mem_do_rinst;
        CData/*1:0*/ __Vdly__mem_wordsize;
        CData/*0:0*/ __Vdly__latched_is_lu;
        CData/*0:0*/ __Vdly__latched_is_lh;
        CData/*0:0*/ __Vdly__latched_is_lb;
        CData/*0:0*/ __Vdly__latched_store;
        CData/*0:0*/ __Vdly__latched_stalu;
        CData/*0:0*/ __Vdly__latched_branch;
        CData/*0:0*/ __Vdly__mem_do_prefetch;
        CData/*7:0*/ __Vdly__cpu_state;
        VL_OUT(mem_addr,31,0);
        VL_OUT(mem_wdata,31,0);
        VL_IN(mem_rdata,31,0);
        VL_OUT(__PVT__mem_la_addr,31,0);
        VL_OUT(__PVT__mem_la_wdata,31,0);
        VL_OUT(pcpi_insn,31,0);
        VL_OUT(pcpi_rs1,31,0);
        VL_OUT(pcpi_rs2,31,0);
        VL_IN(pcpi_rd,31,0);
        VL_IN(irq,31,0);
        VL_OUT(eoi,31,0);
    };
    struct {
        IData/*31:0*/ __PVT__reg_pc;
        IData/*31:0*/ __PVT__reg_next_pc;
        IData/*31:0*/ __PVT__reg_op1;
        IData/*31:0*/ __PVT__reg_op2;
        IData/*31:0*/ __PVT__reg_out;
        IData/*31:0*/ __PVT__next_pc;
        IData/*31:0*/ __PVT__timer;
        IData/*31:0*/ __PVT__mem_rdata_word;
        IData/*31:0*/ __PVT__mem_rdata_q;
        IData/*31:0*/ __PVT__mem_rdata_latched;
        IData/*31:0*/ __PVT__decoded_imm;
        IData/*31:0*/ __PVT__decoded_imm_j;
        IData/*31:0*/ __PVT__current_pc;
        IData/*31:0*/ __PVT__alu_out;
        IData/*31:0*/ __PVT__alu_out_q;
        IData/*31:0*/ __PVT__cpuregs_wrdata;
        IData/*31:0*/ __PVT__cpuregs_rs1;
        IData/*31:0*/ __PVT__cpuregs_rs2;
        IData/*31:0*/ __Vdly__reg_pc;
        IData/*31:0*/ __Vdly__reg_next_pc;
        IData/*31:0*/ __Vdly__reg_op1;
        IData/*31:0*/ __Vdly__timer;
        VL_OUT64(trace_data,35,0);
        QData/*63:0*/ __PVT__count_cycle;
        QData/*63:0*/ __PVT__count_instr;
        QData/*63:0*/ __Vdly__count_instr;
        VlUnpacked<IData/*31:0*/, 32> __PVT__cpuregs;
    };

    // INTERNAL VARIABLES
    Vpicorv32__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vpicorv32_picorv32__pi1(Vpicorv32__Syms* symsp, const char* v__name);
    ~Vpicorv32_picorv32__pi1();
    VL_UNCOPYABLE(Vpicorv32_picorv32__pi1);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
