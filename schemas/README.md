Schema folder for ISA descriptions and mutator configs

Layout (example):

schemas/
├── base.yaml                # Shared field templates, macros, operand rules
├── riscv/
│   ├── riscv32_base.yaml    # Base RV32I
│   ├── riscv32m.yaml        # Adds M-extension
│   ├── riscv32c.yaml        # Adds Compressed extension
│   ├── riscv64_base.yaml    # Base RV64I
│   ├── riscv64m.yaml
│   ├── riscv64gc.yaml
│   └── riscv_vector.yaml
├── armv8/
│   └── armv8a.yaml
└── mips/
    └── mips32.yaml

The YAML files are examples and currently the mutator ships a minimal YAML subset parser.
If you want full YAML features, consider adding yaml-cpp and adapting the mutator.
