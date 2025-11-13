#!/bin/bash
# Verify that all seeds have proper exit stubs

echo "======================================"
echo "Exit Stub Verification Report"
echo "======================================"
echo ""

check_exit_stub() {
    local file=$1
    local name=$(basename "$file")
    
    # Check for store to 0x80001xxx address (tohost)
    if riscv64-unknown-elf-objdump -D -b binary -m riscv:rv32 "$file" 2>/dev/null | grep -q "sw.*0x80001"; then
        echo "✅ $name - Exit stub present"
        return 0
    else
        echo "❌ $name - NO EXIT STUB FOUND"
        return 1
    fi
}

echo "Regular Seeds (seeds/):"
echo "----------------------"
for seed in seeds/*.bin; do
    [ -f "$seed" ] && check_exit_stub "$seed"
done

echo ""
echo "Test Seeds (test_seeds/):"
echo "-------------------------"
for seed in test_seeds/*.bin; do
    [ -f "$seed" ] && check_exit_stub "$seed"
done

echo ""
echo "======================================"
echo "Verification Complete"
echo "======================================"
