// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Primary model header
//
// This header should be included by all source files instantiating the design.
// The class here is then constructed to instantiate the design.
// See the Verilator manual for examples.

#ifndef VERILATED_VPICORV32_H_
#define VERILATED_VPICORV32_H_  // guard

#include "verilated.h"

class Vpicorv32__Syms;
class Vpicorv32___024root;
class Vpicorv32_picorv32__pi1;


// This class is the main interface to the Verilated model
class alignas(VL_CACHE_LINE_BYTES) Vpicorv32 VL_NOT_FINAL : public VerilatedModel {
  private:
    // Symbol table holding complete model state (owned by this class)
    Vpicorv32__Syms* const vlSymsp;

  public:

    // PORTS
    // The application code writes and reads these signals to
    // propagate new values into/out from the Verilated model.
    VL_IN8(&picorv32_regs__02Eclk,0,0);
    VL_IN8(&picorv32_axi__02Eclk,0,0);
    VL_IN8(&wb_clk_i,0,0);
    VL_IN8(&wen,0,0);
    VL_IN8(&waddr,5,0);
    VL_IN8(&raddr1,5,0);
    VL_IN8(&raddr2,5,0);
    VL_IN8(&resetn,0,0);
    VL_OUT8(&picorv32_axi__02Etrap,0,0);
    VL_OUT8(&mem_axi_awvalid,0,0);
    VL_IN8(&mem_axi_awready,0,0);
    VL_OUT8(&mem_axi_awprot,2,0);
    VL_OUT8(&mem_axi_wvalid,0,0);
    VL_IN8(&mem_axi_wready,0,0);
    VL_OUT8(&mem_axi_wstrb,3,0);
    VL_IN8(&mem_axi_bvalid,0,0);
    VL_OUT8(&mem_axi_bready,0,0);
    VL_OUT8(&mem_axi_arvalid,0,0);
    VL_IN8(&mem_axi_arready,0,0);
    VL_OUT8(&mem_axi_arprot,2,0);
    VL_IN8(&mem_axi_rvalid,0,0);
    VL_OUT8(&mem_axi_rready,0,0);
    VL_OUT8(&picorv32_axi__02Epcpi_valid,0,0);
    VL_IN8(&picorv32_axi__02Epcpi_wr,0,0);
    VL_IN8(&picorv32_axi__02Epcpi_wait,0,0);
    VL_IN8(&picorv32_axi__02Epcpi_ready,0,0);
    VL_OUT8(&picorv32_axi__02Etrace_valid,0,0);
    VL_OUT8(&picorv32_wb__02Etrap,0,0);
    VL_IN8(&wb_rst_i,0,0);
    VL_OUT8(&wbm_we_o,0,0);
    VL_OUT8(&wbm_sel_o,3,0);
    VL_OUT8(&wbm_stb_o,0,0);
    VL_IN8(&wbm_ack_i,0,0);
    VL_OUT8(&wbm_cyc_o,0,0);
    VL_OUT8(&picorv32_wb__02Epcpi_valid,0,0);
    VL_IN8(&picorv32_wb__02Epcpi_wr,0,0);
    VL_IN8(&picorv32_wb__02Epcpi_wait,0,0);
    VL_IN8(&picorv32_wb__02Epcpi_ready,0,0);
    VL_OUT8(&picorv32_wb__02Etrace_valid,0,0);
    VL_OUT8(&mem_instr,0,0);
    VL_IN(&wdata,31,0);
    VL_OUT(&rdata1,31,0);
    VL_OUT(&rdata2,31,0);
    VL_OUT(&mem_axi_awaddr,31,0);
    VL_OUT(&mem_axi_wdata,31,0);
    VL_OUT(&mem_axi_araddr,31,0);
    VL_IN(&mem_axi_rdata,31,0);
    VL_OUT(&picorv32_axi__02Epcpi_insn,31,0);
    VL_OUT(&picorv32_axi__02Epcpi_rs1,31,0);
    VL_OUT(&picorv32_axi__02Epcpi_rs2,31,0);
    VL_IN(&picorv32_axi__02Epcpi_rd,31,0);
    VL_IN(&picorv32_axi__02Eirq,31,0);
    VL_OUT(&picorv32_axi__02Eeoi,31,0);
    VL_OUT(&wbm_adr_o,31,0);
    VL_OUT(&wbm_dat_o,31,0);
    VL_IN(&wbm_dat_i,31,0);
    VL_OUT(&picorv32_wb__02Epcpi_insn,31,0);
    VL_OUT(&picorv32_wb__02Epcpi_rs1,31,0);
    VL_OUT(&picorv32_wb__02Epcpi_rs2,31,0);
    VL_IN(&picorv32_wb__02Epcpi_rd,31,0);
    VL_IN(&picorv32_wb__02Eirq,31,0);
    VL_OUT(&picorv32_wb__02Eeoi,31,0);
    VL_OUT64(&picorv32_axi__02Etrace_data,35,0);
    VL_OUT64(&picorv32_wb__02Etrace_data,35,0);

    // CELLS
    // Public to allow access to /* verilator public */ items.
    // Otherwise the application code can consider these internals.
    Vpicorv32_picorv32__pi1* const __PVT__picorv32_axi__DOT__picorv32_core;
    Vpicorv32_picorv32__pi1* const __PVT__picorv32_wb__DOT__picorv32_core;

    // Root instance pointer to allow access to model internals,
    // including inlined /* verilator public_flat_* */ items.
    Vpicorv32___024root* const rootp;

    // CONSTRUCTORS
    /// Construct the model; called by application code
    /// If contextp is null, then the model will use the default global context
    /// If name is "", then makes a wrapper with a
    /// single model invisible with respect to DPI scope names.
    explicit Vpicorv32(VerilatedContext* contextp, const char* name = "TOP");
    explicit Vpicorv32(const char* name = "TOP");
    /// Destroy the model; called (often implicitly) by application code
    virtual ~Vpicorv32();
  private:
    VL_UNCOPYABLE(Vpicorv32);  ///< Copying not allowed

  public:
    // API METHODS
    /// Evaluate the model.  Application must call when inputs change.
    void eval() { eval_step(); }
    /// Evaluate when calling multiple units/models per time step.
    void eval_step();
    /// Evaluate at end of a timestep for tracing, when using eval_step().
    /// Application must call after all eval() and before time changes.
    void eval_end_step() {}
    /// Simulation complete, run final blocks.  Application must call on completion.
    void final();
    /// Are there scheduled events to handle?
    bool eventsPending();
    /// Returns time at next time slot. Aborts if !eventsPending()
    uint64_t nextTimeSlot();
    /// Trace signals in the model; called by application code
    void trace(VerilatedVcdC* tfp, int levels, int options = 0);
    /// Retrieve name of this model instance (as passed to constructor).
    const char* name() const;

    // Abstract methods from VerilatedModel
    const char* hierName() const override final;
    const char* modelName() const override final;
    unsigned threads() const override final;
    /// Prepare for cloning the model at the process level (e.g. fork in Linux)
    /// Release necessary resources. Called before cloning.
    void prepareClone() const;
    /// Re-init after cloning the model at the process level (e.g. fork in Linux)
    /// Re-allocate necessary resources. Called after cloning.
    void atClone() const;
};

#endif  // guard
