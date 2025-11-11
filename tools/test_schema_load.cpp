#include <fuzz/isa/IsaLoader.hpp>
#include <iostream>
#include <iomanip>

void test_isa(const char* isa_name) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Testing ISA: " << isa_name << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        setenv("AFL_ISA_MAP", "schemas/isa_map.yaml", 1);
        auto config = fuzz::isa::load_isa_config(isa_name);
        
        std::cout << "\n✓ Schema loaded successfully!" << std::endl;
        std::cout << "  ISA Name: " << config.isa_name << std::endl;
        std::cout << "  Base Width: " << config.base_width << " bits" << std::endl;
        std::cout << "  Register Count: " << config.register_count << std::endl;
        std::cout << "  Fields: " << config.fields.size() << std::endl;
        std::cout << "  Formats: " << config.formats.size() << std::endl;
        std::cout << "  Instructions: " << config.instructions.size() << std::endl;
        
        // Show first 5 instructions
        std::cout << "\nFirst 5 Instructions:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(5), config.instructions.size()); ++i) {
            const auto& insn = config.instructions[i];
            std::cout << "  " << std::setw(12) << std::left << insn.name 
                      << " format=" << insn.format 
                      << ", fixed=" << insn.fixed_fields.size() << std::endl;
        }
        
        // Check some field types
        std::cout << "\nField Type Summary:" << std::endl;
        int opcode_cnt = 0, reg_cnt = 0, imm_cnt = 0, enum_cnt = 0;
        for (const auto& [name, field] : config.fields) {
            switch (field.kind) {
                case fuzz::isa::FieldKind::Opcode: opcode_cnt++; break;
                case fuzz::isa::FieldKind::Register: reg_cnt++; break;
                case fuzz::isa::FieldKind::Immediate: imm_cnt++; break;
                case fuzz::isa::FieldKind::Enum: enum_cnt++; break;
                default: break;
            }
        }
        std::cout << "  Opcode fields: " << opcode_cnt << std::endl;
        std::cout << "  Register fields: " << reg_cnt << std::endl;
        std::cout << "  Immediate fields: " << imm_cnt << std::endl;
        std::cout << "  Enum/funct fields: " << enum_cnt << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error loading " << isa_name << ": " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "ISA Schema Loader Test" << std::endl;
    
    test_isa("rv32i");
    test_isa("rv32im");
    test_isa("rv32imc");
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "All tests completed!" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    return 0;
}
