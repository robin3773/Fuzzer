// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vpicorv32.h for the primary calling header

#ifndef VERILATED_VPICORV32___024ROOT_H_
#define VERILATED_VPICORV32___024ROOT_H_  // guard

#include "verilated.h"
class Vpicorv32_picorv32__pi1;


class Vpicorv32__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vpicorv32___024root final : public VerilatedModule {
  public:
    // CELLS
    Vpicorv32_picorv32__pi1* __PVT__picorv32_axi__DOT__picorv32_core;
    Vpicorv32_picorv32__pi1* __PVT__picorv32_wb__DOT__picorv32_core;

    // DESIGN SPECIFIC STATE
    // Anonymous structures to workaround compiler member-count bugs
    struct {
        VL_IN8(picorv32_regs__02Eclk,0,0);
        VL_IN8(picorv32_axi__02Eclk,0,0);
        VL_IN8(wb_clk_i,0,0);
        VL_IN8(wen,0,0);
        VL_IN8(waddr,5,0);
        VL_IN8(raddr1,5,0);
        VL_IN8(raddr2,5,0);
        VL_IN8(resetn,0,0);
        VL_OUT8(picorv32_axi__02Etrap,0,0);
        VL_OUT8(mem_axi_awvalid,0,0);
        VL_IN8(mem_axi_awready,0,0);
        VL_OUT8(mem_axi_awprot,2,0);
        VL_OUT8(mem_axi_wvalid,0,0);
        VL_IN8(mem_axi_wready,0,0);
        VL_OUT8(mem_axi_wstrb,3,0);
        VL_IN8(mem_axi_bvalid,0,0);
        VL_OUT8(mem_axi_bready,0,0);
        VL_OUT8(mem_axi_arvalid,0,0);
        VL_IN8(mem_axi_arready,0,0);
        VL_OUT8(mem_axi_arprot,2,0);
        VL_IN8(mem_axi_rvalid,0,0);
        VL_OUT8(mem_axi_rready,0,0);
        VL_OUT8(picorv32_axi__02Epcpi_valid,0,0);
        VL_IN8(picorv32_axi__02Epcpi_wr,0,0);
        VL_IN8(picorv32_axi__02Epcpi_wait,0,0);
        VL_IN8(picorv32_axi__02Epcpi_ready,0,0);
        VL_OUT8(picorv32_axi__02Etrace_valid,0,0);
        VL_OUT8(picorv32_wb__02Etrap,0,0);
        VL_IN8(wb_rst_i,0,0);
        VL_OUT8(wbm_we_o,0,0);
        VL_OUT8(wbm_sel_o,3,0);
        VL_OUT8(wbm_stb_o,0,0);
        VL_IN8(wbm_ack_i,0,0);
        VL_OUT8(wbm_cyc_o,0,0);
        VL_OUT8(picorv32_wb__02Epcpi_valid,0,0);
        VL_IN8(picorv32_wb__02Epcpi_wr,0,0);
        VL_IN8(picorv32_wb__02Epcpi_wait,0,0);
        VL_IN8(picorv32_wb__02Epcpi_ready,0,0);
        VL_OUT8(picorv32_wb__02Etrace_valid,0,0);
        VL_OUT8(mem_instr,0,0);
        CData/*0:0*/ picorv32_axi__DOT__mem_ready;
        CData/*0:0*/ picorv32_axi__DOT__axi_adapter__DOT__ack_awvalid;
        CData/*0:0*/ picorv32_axi__DOT__axi_adapter__DOT__ack_arvalid;
        CData/*0:0*/ picorv32_axi__DOT__axi_adapter__DOT__ack_wvalid;
        CData/*0:0*/ picorv32_axi__DOT__axi_adapter__DOT__xfer_done;
        CData/*0:0*/ picorv32_wb__DOT__mem_ready;
        CData/*1:0*/ picorv32_wb__DOT__state;
        CData/*0:0*/ __Vdly__picorv32_wb__DOT__mem_ready;
        CData/*0:0*/ __VstlFirstIteration;
        CData/*0:0*/ __VicoFirstIteration;
        CData/*0:0*/ __Vtrigprevexpr___TOP__picorv32_regs__02Eclk__0;
        CData/*0:0*/ __Vtrigprevexpr___TOP__picorv32_axi__02Eclk__0;
        CData/*0:0*/ __Vtrigprevexpr___TOP__wb_clk_i__0;
        CData/*0:0*/ __VactContinue;
        VL_IN(wdata,31,0);
        VL_OUT(rdata1,31,0);
        VL_OUT(rdata2,31,0);
        VL_OUT(mem_axi_awaddr,31,0);
        VL_OUT(mem_axi_wdata,31,0);
        VL_OUT(mem_axi_araddr,31,0);
        VL_IN(mem_axi_rdata,31,0);
        VL_OUT(picorv32_axi__02Epcpi_insn,31,0);
        VL_OUT(picorv32_axi__02Epcpi_rs1,31,0);
        VL_OUT(picorv32_axi__02Epcpi_rs2,31,0);
    };
    struct {
        VL_IN(picorv32_axi__02Epcpi_rd,31,0);
        VL_IN(picorv32_axi__02Eirq,31,0);
        VL_OUT(picorv32_axi__02Eeoi,31,0);
        VL_OUT(wbm_adr_o,31,0);
        VL_OUT(wbm_dat_o,31,0);
        VL_IN(wbm_dat_i,31,0);
        VL_OUT(picorv32_wb__02Epcpi_insn,31,0);
        VL_OUT(picorv32_wb__02Epcpi_rs1,31,0);
        VL_OUT(picorv32_wb__02Epcpi_rs2,31,0);
        VL_IN(picorv32_wb__02Epcpi_rd,31,0);
        VL_IN(picorv32_wb__02Eirq,31,0);
        VL_OUT(picorv32_wb__02Eeoi,31,0);
        IData/*31:0*/ picorv32_regs__DOT____Vlvbound_hf23fd5e2__0;
        IData/*31:0*/ picorv32_wb__DOT__mem_rdata;
        IData/*31:0*/ __Vdly__picorv32_wb__DOT__mem_rdata;
        IData/*31:0*/ __VactIterCount;
        VL_OUT64(picorv32_axi__02Etrace_data,35,0);
        VL_OUT64(picorv32_wb__02Etrace_data,35,0);
        VlUnpacked<IData/*31:0*/, 31> picorv32_regs__DOT__regs;
    };
    VlTriggerVec<1> __VstlTriggered;
    VlTriggerVec<1> __VicoTriggered;
    VlTriggerVec<3> __VactTriggered;
    VlTriggerVec<3> __VnbaTriggered;

    // INTERNAL VARIABLES
    Vpicorv32__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vpicorv32___024root(Vpicorv32__Syms* symsp, const char* v__name);
    ~Vpicorv32___024root();
    VL_UNCOPYABLE(Vpicorv32___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
