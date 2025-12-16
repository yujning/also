// #ifndef BI_D_HPP
// #define BI_D_HPP

// #include <iostream>
// #include <iomanip>
// #include <cmath>
// #include <alice/alice.hpp>
// #include <kitty/constructors.hpp>
// #include <kitty/dynamic_truth_table.hpp>
// #include <kitty/print.hpp>

// // 你的 STP / DSD 头文件（里面有 struct TT 的定义）
// #include "../include/algorithms/stp_dsd.hpp"

// // 上面这个头要先包含，然后再包含 bi_decomposition.hpp
// #include "../include/algorithms/bi_decomposition.hpp"

// namespace alice
// {

// class bd_command : public command
// {
// public:
//     explicit bd_command(const environment::ptr &env)
//         : command(env, "Bi-decomposition (no reorder, k2=1)")
//     {
//         add_option("-f, --factor", hex_input,
//                    "hexadecimal number (must map to 2^n bits)");
//     }

// protected:
//     void execute() override
//     {
//         using clk = std::chrono::high_resolution_clock;

//         if (!is_set("factor"))
//         {
//             std::cout << "❌ Please use: bd -f <hex>\n";
//             return;
//         }

//         std::string hex = hex_input;

//         // 去掉 0x 前缀
//         if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0)
//             hex = hex.substr(2);

//         unsigned bit_count = hex.size() * 4;
//         if (bit_count == 0)
//         {
//             std::cout << "❌ Empty hex string.\n";
//             return;
//         }

//         unsigned num_vars = 0;
//         while ((1u << num_vars) < bit_count) num_vars++;
//         if ((1u << num_vars) != bit_count)
//         {
//             std::cout << "❌ Hex length is not 2^n bits.\n";
//             return;
//         }

//         if (bit_count == 4)
//         {
//             std::cout << "⚠ Input is already 2-input, nothing to decompose.\n";
//             return;
//         }

//         kitty::dynamic_truth_table tt(num_vars);
//         kitty::create_from_hex_string(tt, hex);

//         std::ostringstream oss;
//         kitty::print_binary(tt, oss);
//         std::string binary = oss.str();

//         std::cout << "📘 Hex = " << hex
//                   << "  =>  binary = " << binary
//                   << " (vars = " << num_vars << ")\n";

//         // 构造 TT
//         TT in;
//         in.f01 = binary;
//         in.order.resize(num_vars);
//         for (unsigned i = 0; i < num_vars; ++i)
//             in.order[i] = i + 1;  // 变量按 1..n

//         auto t1 = clk::now();
//         auto all_res = enumerate_bi_decomposition_all_permutations(in);

//         auto t2 = clk::now();
//         auto us =
//             std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

// if (all_res.empty())
// {
//     std::cout << "\n❌ No bi-decomposition found for any k2,k3.\n";
// }
// else
// {
//     std::cout << "\n=== Found " << all_res.size()
//               << " bi-decomposition(s) (minimal k2 = " << all_res[0].k2 << ") ===\n\n";

//     int id = 0;
//     for (auto &r : all_res)
//     {
//         std::cout << "--------------------------------------------\n";
//         std::cout << "Decomposition #" << (++id) << "\n";
//         std::cout << "k1=" << r.k1 << "  k2=" << r.k2 << "  k3=" << r.k3 << "\n";

//         std::cout << "Γ = { ";
//         for (int v : r.Gamma) std::cout << v << " ";
//         std::cout << "}\n";

//         std::cout << "Θ = { ";
//         for (int v : r.Theta) std::cout << v << " ";
//         std::cout << "}\n";

//         std::cout << "Λ = { ";
//         for (int v : r.Lambda) std::cout << v << " ";
//         std::cout << "}\n";

//         std::cout << "F(u,v) = " << r.F01
//                   << "   (order: 00,01,10,11)\n";

//         std::cout << "φ-table = " << r.phi_tt.f01 << "   order: ";
//         for (int v : r.phi_tt.order) std::cout << v << " ";
//         std::cout << "\n";

//         std::cout << "ψ-table = " << r.psi_tt.f01 << "   order: ";
//         for (int v : r.psi_tt.order) std::cout << v << " ";
//         std::cout << "\n";
//     }
// }


//         std::cout << "\n⏱ execution time = " << us << " us\n";
//     }

// private:
//     std::string hex_input{};
// };

// ALICE_ADD_COMMAND(bd, "STP")

// } // namespace alice

// #endif

#ifndef BI_D_HPP
#define BI_D_HPP

#include <iostream>
#include <iomanip>
#include <cmath>
#include <alice/alice.hpp>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>

#include "../include/algorithms/node_global.hpp"
#include "../include/algorithms/stp_dsd.hpp"
#include "../include/algorithms/bi_decomposition.hpp"
#include "../include/algorithms/bi_dec_else_dec.hpp"   

namespace alice
{

class bd_command : public command
{
public:
    explicit bd_command(const environment::ptr &env)
        : command(env, "Bi-decomposition (recursive)")
    {
        add_option("-f, --factor", hex_input,
                   "truth table as hex string")->required();

        // ⭐ 加 else dec fallback 开关
        add_flag("-e, --else_dec", use_else_dec,
                 "enable else_dec fallback when BD fails");
    }

protected:
    void execute() override
    {
        using clk = std::chrono::high_resolution_clock;
        
        use_else_dec = is_set("else_dec");
        std::string hex = hex_input;

        if (!is_set("factor"))
        {
            std::cout << "❌ Usage: bd -f <hex> [-s]\n";
            return;
        }

        // ------------------------------------------------------
        // 解析 Hex 变成真值表（binary）
        // ------------------------------------------------------
        if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0)
            hex = hex.substr(2);

        unsigned bits = hex.size() * 4;
        if (bits == 0)
        {
            std::cout << "❌ Empty truth table.\n";
            return;
        }

        unsigned nvars = 0;
        while ((1u << nvars) < bits) nvars++;

        if ((1u << nvars) != bits)
        {
            std::cout << "❌ TT size is not 2^n.\n";
            return;
        }

        kitty::dynamic_truth_table tt(nvars);
        kitty::create_from_hex_string(tt, hex);

        std::ostringstream oss;
        kitty::print_binary(tt, oss);
        std::string binF = oss.str();

        std::cout << "📘 TT = " << binF << "  (vars=" << nvars << ")\n";

        // ------------------------------------------------------
        // 设置 其他分解模式（全局变量）
        // ------------------------------------------------------
        ENABLE_ELSE_DEC = use_else_dec;  // ⭐ 关键一步

        auto t1 = clk::now();

        
        bool success = run_bi_decomp_recursive(binF);

        auto t2 = clk::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

        if (!success)
            std::cout << "❌ Decomposition failed\n";

        std::cout << "⏱ time = " << us << " us\n";
    }

private:
    std::string hex_input{};
    bool use_else_dec = false;  // ⭐ 是否启用 其他分解
};

ALICE_ADD_COMMAND(bd, "STP")

} // namespace alice

#endif
