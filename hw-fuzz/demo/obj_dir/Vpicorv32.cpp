// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vpicorv32__pch.h"

//============================================================
// Constructors

Vpicorv32::Vpicorv32(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Vpicorv32__Syms(contextp(), _vcname__, this)}
    , picorv32_regs__02Eclk{vlSymsp->TOP.picorv32_regs__02Eclk}
    , picorv32_axi__02Eclk{vlSymsp->TOP.picorv32_axi__02Eclk}
    , wb_clk_i{vlSymsp->TOP.wb_clk_i}
    , wen{vlSymsp->TOP.wen}
    , waddr{vlSymsp->TOP.waddr}
    , raddr1{vlSymsp->TOP.raddr1}
    , raddr2{vlSymsp->TOP.raddr2}
    , resetn{vlSymsp->TOP.resetn}
    , picorv32_axi__02Etrap{vlSymsp->TOP.picorv32_axi__02Etrap}
    , mem_axi_awvalid{vlSymsp->TOP.mem_axi_awvalid}
    , mem_axi_awready{vlSymsp->TOP.mem_axi_awready}
    , mem_axi_awprot{vlSymsp->TOP.mem_axi_awprot}
    , mem_axi_wvalid{vlSymsp->TOP.mem_axi_wvalid}
    , mem_axi_wready{vlSymsp->TOP.mem_axi_wready}
    , mem_axi_wstrb{vlSymsp->TOP.mem_axi_wstrb}
    , mem_axi_bvalid{vlSymsp->TOP.mem_axi_bvalid}
    , mem_axi_bready{vlSymsp->TOP.mem_axi_bready}
    , mem_axi_arvalid{vlSymsp->TOP.mem_axi_arvalid}
    , mem_axi_arready{vlSymsp->TOP.mem_axi_arready}
    , mem_axi_arprot{vlSymsp->TOP.mem_axi_arprot}
    , mem_axi_rvalid{vlSymsp->TOP.mem_axi_rvalid}
    , mem_axi_rready{vlSymsp->TOP.mem_axi_rready}
    , picorv32_axi__02Epcpi_valid{vlSymsp->TOP.picorv32_axi__02Epcpi_valid}
    , picorv32_axi__02Epcpi_wr{vlSymsp->TOP.picorv32_axi__02Epcpi_wr}
    , picorv32_axi__02Epcpi_wait{vlSymsp->TOP.picorv32_axi__02Epcpi_wait}
    , picorv32_axi__02Epcpi_ready{vlSymsp->TOP.picorv32_axi__02Epcpi_ready}
    , picorv32_axi__02Etrace_valid{vlSymsp->TOP.picorv32_axi__02Etrace_valid}
    , picorv32_wb__02Etrap{vlSymsp->TOP.picorv32_wb__02Etrap}
    , wb_rst_i{vlSymsp->TOP.wb_rst_i}
    , wbm_we_o{vlSymsp->TOP.wbm_we_o}
    , wbm_sel_o{vlSymsp->TOP.wbm_sel_o}
    , wbm_stb_o{vlSymsp->TOP.wbm_stb_o}
    , wbm_ack_i{vlSymsp->TOP.wbm_ack_i}
    , wbm_cyc_o{vlSymsp->TOP.wbm_cyc_o}
    , picorv32_wb__02Epcpi_valid{vlSymsp->TOP.picorv32_wb__02Epcpi_valid}
    , picorv32_wb__02Epcpi_wr{vlSymsp->TOP.picorv32_wb__02Epcpi_wr}
    , picorv32_wb__02Epcpi_wait{vlSymsp->TOP.picorv32_wb__02Epcpi_wait}
    , picorv32_wb__02Epcpi_ready{vlSymsp->TOP.picorv32_wb__02Epcpi_ready}
    , picorv32_wb__02Etrace_valid{vlSymsp->TOP.picorv32_wb__02Etrace_valid}
    , mem_instr{vlSymsp->TOP.mem_instr}
    , wdata{vlSymsp->TOP.wdata}
    , rdata1{vlSymsp->TOP.rdata1}
    , rdata2{vlSymsp->TOP.rdata2}
    , mem_axi_awaddr{vlSymsp->TOP.mem_axi_awaddr}
    , mem_axi_wdata{vlSymsp->TOP.mem_axi_wdata}
    , mem_axi_araddr{vlSymsp->TOP.mem_axi_araddr}
    , mem_axi_rdata{vlSymsp->TOP.mem_axi_rdata}
    , picorv32_axi__02Epcpi_insn{vlSymsp->TOP.picorv32_axi__02Epcpi_insn}
    , picorv32_axi__02Epcpi_rs1{vlSymsp->TOP.picorv32_axi__02Epcpi_rs1}
    , picorv32_axi__02Epcpi_rs2{vlSymsp->TOP.picorv32_axi__02Epcpi_rs2}
    , picorv32_axi__02Epcpi_rd{vlSymsp->TOP.picorv32_axi__02Epcpi_rd}
    , picorv32_axi__02Eirq{vlSymsp->TOP.picorv32_axi__02Eirq}
    , picorv32_axi__02Eeoi{vlSymsp->TOP.picorv32_axi__02Eeoi}
    , wbm_adr_o{vlSymsp->TOP.wbm_adr_o}
    , wbm_dat_o{vlSymsp->TOP.wbm_dat_o}
    , wbm_dat_i{vlSymsp->TOP.wbm_dat_i}
    , picorv32_wb__02Epcpi_insn{vlSymsp->TOP.picorv32_wb__02Epcpi_insn}
    , picorv32_wb__02Epcpi_rs1{vlSymsp->TOP.picorv32_wb__02Epcpi_rs1}
    , picorv32_wb__02Epcpi_rs2{vlSymsp->TOP.picorv32_wb__02Epcpi_rs2}
    , picorv32_wb__02Epcpi_rd{vlSymsp->TOP.picorv32_wb__02Epcpi_rd}
    , picorv32_wb__02Eirq{vlSymsp->TOP.picorv32_wb__02Eirq}
    , picorv32_wb__02Eeoi{vlSymsp->TOP.picorv32_wb__02Eeoi}
    , picorv32_axi__02Etrace_data{vlSymsp->TOP.picorv32_axi__02Etrace_data}
    , picorv32_wb__02Etrace_data{vlSymsp->TOP.picorv32_wb__02Etrace_data}
    , __PVT__picorv32_axi__DOT__picorv32_core{vlSymsp->TOP.__PVT__picorv32_axi__DOT__picorv32_core}
    , __PVT__picorv32_wb__DOT__picorv32_core{vlSymsp->TOP.__PVT__picorv32_wb__DOT__picorv32_core}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
}

Vpicorv32::Vpicorv32(const char* _vcname__)
    : Vpicorv32(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Vpicorv32::~Vpicorv32() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Vpicorv32___024root___eval_debug_assertions(Vpicorv32___024root* vlSelf);
#endif  // VL_DEBUG
void Vpicorv32___024root___eval_static(Vpicorv32___024root* vlSelf);
void Vpicorv32___024root___eval_initial(Vpicorv32___024root* vlSelf);
void Vpicorv32___024root___eval_settle(Vpicorv32___024root* vlSelf);
void Vpicorv32___024root___eval(Vpicorv32___024root* vlSelf);

void Vpicorv32::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vpicorv32::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vpicorv32___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        vlSymsp->__Vm_didInit = true;
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Vpicorv32___024root___eval_static(&(vlSymsp->TOP));
        Vpicorv32___024root___eval_initial(&(vlSymsp->TOP));
        Vpicorv32___024root___eval_settle(&(vlSymsp->TOP));
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Vpicorv32___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Vpicorv32::eventsPending() { return false; }

uint64_t Vpicorv32::nextTimeSlot() {
    VL_FATAL_MT(__FILE__, __LINE__, "", "%Error: No delays in the design");
    return 0;
}

//============================================================
// Utilities

const char* Vpicorv32::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Vpicorv32___024root___eval_final(Vpicorv32___024root* vlSelf);

VL_ATTR_COLD void Vpicorv32::final() {
    Vpicorv32___024root___eval_final(&(vlSymsp->TOP));
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Vpicorv32::hierName() const { return vlSymsp->name(); }
const char* Vpicorv32::modelName() const { return "Vpicorv32"; }
unsigned Vpicorv32::threads() const { return 1; }
void Vpicorv32::prepareClone() const { contextp()->prepareClone(); }
void Vpicorv32::atClone() const {
    contextp()->threadPoolpOnClone();
}

//============================================================
// Trace configuration

VL_ATTR_COLD void Vpicorv32::trace(VerilatedVcdC* tfp, int levels, int options) {
    vl_fatal(__FILE__, __LINE__, __FILE__,"'Vpicorv32::trace()' called on model that was Verilated without --trace option");
}
