#ifndef DSD_HPP
#define DSD_HPP

#include <iostream>
#include <iomanip>
#include <cmath>
#include <bitset>
#include <alice/alice.hpp>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>

// 引入算法
#include "../include/algorithms/stp_dsd.hpp"
#include "../include/algorithms/reorder.hpp"   // all_reorders_char(raw)

namespace alice
{
    class dsd_command : public command
    {
    public:
        explicit dsd_command(const environment::ptr &env)
            : command(env, "Run STP decomposition / raw reorder")
        {
            add_option("-f, --factor", hex_input,
                       "hexadecimal number (must map to 2^n bits)");

            add_option("-x, --raw", raw_input,
                       "raw truth-table with don't care (x), length must be 2^n");
        }

    protected:
  void execute() override
{
    using clk = std::chrono::high_resolution_clock;

    bool use_raw = is_set("raw");
    bool use_hex = is_set("factor");

    if (use_raw && use_hex)
    {
        std::cout << "❌ Options -f and -x cannot be used together.\n";
        return;
    }

    // ------------ RAW MODE ------------
    if (use_raw)
    {
        std::string raw = raw_input;

        if (!is_power_of_two(raw.size()))
        {
            std::cout << "❌ Error: length (" << raw.size() << ") is not power of 2\n";
            return;
        }
         
        if (raw.size() == 4) {
           std::cout << "⚠ 输入函数已是 2-LUT，无需分解。\n";
        return;
        }
        std::cout << "➡ Raw truth-table mode (-x)\n";
        std::cout << "Input = " << raw << "\n";

        auto t1 = clk::now();
        all_reorders_char(raw);
        auto t2 = clk::now();

        auto us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        std::cout << "⏱ RAW decomposition time = " << us << " us\n";

        return;
    }

    // ------------ HEX MODE ------------
    if (!use_hex)
    {
        std::cout << "❌ Please use -f <hex> or -x <raw>\n";
        return;
    }

    std::string hex = hex_input;

    if (hex.rfind("0x",0)==0 || hex.rfind("0X",0)==0)
        hex = hex.substr(2);

    unsigned bit_count = hex.size() * 4;
    unsigned num_vars = 0;
    while ((1u << num_vars) < bit_count) num_vars++;

    if ((1u << num_vars) != bit_count)
    {
        std::cout << "❌ Hex length is not 2^n bits\n";
        return;
    }

        if (bit_count == 4) {
        std::cout << "⚠ 输入函数已是 2-LUT，无需分解。\n";
        return;
    }

    kitty::dynamic_truth_table tt(num_vars);
    kitty::create_from_hex_string(tt, hex);

    std::ostringstream oss;
    kitty::print_binary(tt, oss);
    std::string binary = oss.str();

    std::cout << "📘 Hex " << hex << " => binary " << binary
              << " (len = " << binary.size() << " vars = " << num_vars << ")\n";

    // 计时
    auto t1 = clk::now();
    //all_reorders(binary);
    run_dsd_recursive(binary);
    auto t2 = clk::now();

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "⏱ DSD execution time = " << us << " us\n";
}

    private:
        std::string hex_input{};
        std::string raw_input{};
    };

    ALICE_ADD_COMMAND(dsd, "STP")
}

#endif
