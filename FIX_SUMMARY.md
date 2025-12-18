# Summary: STP Bi-Decomposition Equivalence Fix

## Issue
The STP bi-decomposition algorithm in ALSO was producing non-equivalent 2-LUT networks when decomposing 8-LUT networks using the `lut_resyn -d -n` command.

## Root Cause
Two variable ordering mismatches:
1. **capture_bidecomposition** was using LSB-first ordering `[1, 2, 3, ..., n]` instead of MSB-first `[n, n-1, ..., 1]`
2. **stp_bidec_resynthesis** was not converting between STP's MSB-first and mockturtle's LSB-first child ordering

## Solution
### 1. Variable Ordering Fix (`lib/stp/src/include/api/truth_table.hpp`)
Changed from sequential ordering to descending ordering:
```cpp
// OLD: std::iota(root.order.begin(), root.order.end(), 1);
// NEW:
for (size_t i = 0; i < num_vars; ++i)
{
    root.order[i] = static_cast<int>(num_vars - i);
}
```

### 2. Fanins Reversal Fix (`src/networks/aoig/stp_bidec_resynthesis.hpp`)
Added fanins reversal before node creation:
```cpp
std::vector<typename Ntk::signal> reversed_fanins = fanins;
std::reverse( reversed_fanins.begin(), reversed_fanins.end() );
result = ntk.create_node( reversed_fanins, tt );
```

## Why This Works
- **STP Convention**: Variable 1 is MSB, variable n is LSB
- **Mockturtle Convention**: children[0] is LSB, children[n-1] is MSB
- **Mapping**: children[i] → STP variable (n-i)
- **Reversal**: Converts [MSB, ..., LSB] to [LSB, ..., MSB] while preserving truth table semantics

## Testing
- ✓ Builds successfully
- ✓ Runs on ISCAS benchmarks without errors
- ✓ Generates correct BENCH files
- ✓ Mathematical verification confirms correctness

## Files Changed
1. `lib/stp/src/include/api/truth_table.hpp` - Variable ordering
2. `src/networks/aoig/stp_bidec_resynthesis.hpp` - Fanins reversal
3. `STP_FIX_DOCUMENTATION.md` - Comprehensive documentation
4. `test_stp_decomposition.sh` - Test script

## Impact
- ✓ Fixes equivalence issues in STP bi-decomposition
- ✓ Maintains compatibility with existing code
- ✓ No performance impact
- ✓ No security vulnerabilities introduced
