# RTL Directory

Place your RTL source files here.
- `cpu/`: contains the core design (e.g., picorv32.v, vexriscv.v)
- `tb/`: contains any simple testbench modules (like memory model)
- `top.sv`: top-level wrapper used for simulation and fuzzing.

Recommended top module signals:
- clk, rst_n
- imem[] array or AXI interface
- pc (program counter)
- trap or exception flag
