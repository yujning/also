#!/bin/bash
# Test script to verify STP bi-decomposition equivalence fix
# This tests the lut_resyn -d command with various benchmarks

cd "$(dirname "$0")/build"

echo "======================================================"
echo "Testing STP bi-decomposition with ALSO"
echo "======================================================"

# Test with c17 benchmark
echo ""
echo "Test 1: c17 benchmark with k=4 LUT mapping and STP decomposition"
./bin/also -c "
read_aiger ../test/aiger_benchmarks/c17.aig;
ps;
lut_mapping -k 4;
ps -l;
lut_resyn -d -n;
ps -l;
quit
" 2>&1 | grep -E "(i/o|and|lev|KLUT)"

echo ""
echo "Test 2: c432 benchmark"
./bin/also -c "
read_aiger ../test/aiger_benchmarks/c432.aig;
ps;
lut_mapping -k 6;
ps -l;
lut_resyn -d -n;
ps -l;
quit
" 2>&1 | grep -E "(i/o|and|lev|KLUT)"

echo ""
echo "======================================================"
echo "Tests completed successfully!"
echo "======================================================"
