#pragma once

#include <sstream>
#include <string>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>

#include "../algorithms/bi_decomposition.hpp"
#include "../algorithms/stp_dsd.hpp"

namespace stp
{

/**
 * Run DSD decomposition directly from a binary truth table string.
 *
 * @param binary_truth_table Truth table encoded as a binary string whose
 *                           length is a power of two.
 * @return true when the input is valid and the run succeeds.
 */
inline bool run_dsd_from_binary(const std::string& binary_truth_table)
{
    return run_dsd_recursive(binary_truth_table);
}

/**
 * Run DSD decomposition from a hexadecimal truth table string.
 *
 * @param hex_truth_table Hexadecimal string representing a truth table.
 * @return true when the input is valid and the run succeeds.
 */
inline bool run_dsd_from_hex(const std::string& hex_truth_table)
{
    auto hex = hex_truth_table;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0)
    {
        hex = hex.substr(2);
    }

    const auto bit_count = static_cast<unsigned>(hex.size() * 4);

    unsigned num_vars = 0;
    while ((1u << num_vars) < bit_count) ++num_vars;
    if ((1u << num_vars) != bit_count)
    {
        return false;
    }

    kitty::dynamic_truth_table tt(num_vars);
    kitty::create_from_hex_string(tt, hex);

    std::ostringstream oss;
    kitty::print_binary(tt, oss);
    return run_dsd_recursive(oss.str());
}

/**
 * Run bi-decomposition directly from a binary truth table string.
 *
 * @param binary_truth_table Truth table encoded as a binary string whose
 *                           length is a power of two.
 * @return true when the input is valid and the run succeeds.
 */
inline bool run_bidecomposition_from_binary(const std::string& binary_truth_table)
{
    return run_bi_decomp_recursive(binary_truth_table);
}

/**
 * Run bi-decomposition directly from a kitty truth table.
 *
 * @param tt kitty dynamic truth table.
 * @return true when the input is valid and the run succeeds.
 */
inline bool run_bidecomposition(const kitty::dynamic_truth_table& tt)
{
    std::ostringstream oss;
    kitty::print_binary(tt, oss);
    return run_bi_decomp_recursive(oss.str());
}

/**
 * Run DSD directly from a kitty truth table.
 *
 * @param tt kitty dynamic truth table.
 * @return true when the input is valid and the run succeeds.
 */
inline bool run_dsd(const kitty::dynamic_truth_table& tt)
{
    std::ostringstream oss;
    kitty::print_binary(tt, oss);
    return run_dsd_recursive(oss.str());
}

} // namespace stp
