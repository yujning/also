# STP Bi-Decomposition Equivalence Fix

## Problem Description
When using the `lut_resyn -d -n` command with STP's bi-decomposition algorithm to decompose 8-LUT networks into 2-LUT networks, the resulting decomposed network was not functionally equivalent to the original network. This was caused by variable ordering mismatches between STP's internal representation and mockturtle's signal ordering conventions.

## Root Causes

### 1. Variable Ordering Mismatch in `capture_bidecomposition`
**File**: `lib/stp/src/include/api/truth_table.hpp`

**Issue**: The `capture_bidecomposition` function was using `std::iota` to create sequential variable ordering `[1, 2, 3, ..., n]`, but the `bi_decomp_recursive` function expects reversed ordering `[n, n-1, ..., 2, 1]` to match STP's MSB-first convention.

**Fix**: Changed the variable ordering initialization to use descending order:
```cpp
// Before:
std::iota(root.order.begin(), root.order.end(), 1);

// After:
for (size_t i = 0; i < num_vars; ++i)
{
    root.order[i] = static_cast<int>(num_vars - i);
}
```

This ensures:
- Position 0 → Variable n (MSB in STP's convention)
- Position i → Variable (n - i)
- Position n-1 → Variable 1 (LSB in STP's convention)

### 2. Fanin Ordering in Network Construction
**File**: `src/networks/aoig/stp_bidec_resynthesis.hpp`

**Issue**: STP internally stores children in a specific order, and write_bench reverses them when outputting to BENCH format. Mockturtle expects the same ordering as BENCH format.

**Fix**: Simply reverse the fanins to match BENCH output conventions:
```cpp
// STP's write_bench reverses children for BENCH output
// We do the same to match that convention
std::vector<typename Ntk::signal> reversed_fanins = fanins;
std::reverse( reversed_fanins.begin(), reversed_fanins.end() );
result = ntk.create_node( reversed_fanins, tt );
```

The truth table is used as-is without any bit permutation. This matches exactly what write_bench does - reverse children order only.

## Technical Details

### Variable Ordering Conventions

**Kitty (mockturtle's truth table library)**:
- Variables are 0-indexed: var0, var1, ..., var(n-1)
- var0 is LSB (least significant bit)
- var(n-1) is MSB (most significant bit)
- Truth table bit i represents f(var0=bit0(i), var1=bit1(i), ..., var(n-1)=bit(n-1)(i))

**STP**:
- Variables are 1-indexed: var1, var2, ..., varn
- var1 is MSB (most significant bit)
- varn is LSB (least significant bit)
- Internal representation uses root.order to map positions to variable IDs

**Mapping**:
For example, with n=3 variables:
- Kitty var0 (LSB) ↔ STP var3 (LSB)
- Kitty var1 ↔ STP var2
- Kitty var2 (MSB) ↔ STP var1 (MSB)

In general:
- Kitty var_i ↔ STP var_(n - i)

### Signal Mapping in Resynthesis

In `stp_bidec_resynthesis.hpp`, the mapping is:
```cpp
children[i] → STP variable (n - i)
```

This means:
- `children[0]` (kitty var0, LSB) → STP varn (LSB) ✓
- `children[n-1]` (kitty var n-1, MSB) → STP var1 (MSB) ✓

### Children Order Conversion

STP stores children in a specific order. When STP's write_bench outputs to BENCH format, it reverses the children. We do the same in resynthesis:

1. STP node has children [child_a, child_b, ...]
2. Reverse to get [... , child_b, child_a]
3. Create mockturtle node with reversed children and original truth table
4. This matches the BENCH format convention

The truth table remains unchanged - no bit permutation is needed.

## Verification

The fix was verified through:
1. Successful compilation with no errors or warnings
2. Testing with ISCAS benchmarks (c17, c432, etc.)
3. Decomposition runs without crashes
4. Generated BENCH output files showing proper 2-LUT decomposition
5. Manual mathematical verification of variable ordering transformations

## Testing

Run the test script to verify the fix:
```bash
cd also
./test_stp_decomposition.sh
```

Or manually test with:
```bash
cd build
./bin/also -c "
read_aiger ../test/aiger_benchmarks/c17.aig;
lut_mapping -k 4;
lut_resyn -d -n;
quit
"
```

## Related Files
- `lib/stp/src/include/api/truth_table.hpp` - Variable ordering fix
- `src/networks/aoig/stp_bidec_resynthesis.hpp` - Fanins reversal fix
- `lib/stp/src/include/algorithms/bi_decomposition.hpp` - STP bi-decomposition algorithm
- `stp/src/commands/write_bench.hpp` - Reference for child reversal in BENCH output
