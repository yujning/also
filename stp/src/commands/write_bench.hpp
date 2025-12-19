#ifndef WRITE_BENCH_HPP
#define WRITE_BENCH_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <alice/alice.hpp>

// 来自 DSD 的节点 & 变量顺序
extern std::vector<DSDNode> NODE_LIST;
extern inline std::vector<int> FINAL_VAR_ORDER;
extern inline int ORIGINAL_VAR_COUNT;

namespace alice
{

// ========================================================
// 变量编号 → 字母
// varN → 'a', var(N-1) → 'b', …
// ========================================================
static std::string varname_from_id(int v)
{
    // int maxv = ORIGINAL_VAR_COUNT;
    // return std::string(1, char('a' + (maxv - v)));
        return std::string(1, char('a' + (v - 1)));
}

// ========================================================
// 安全二进制转 hex（任意长度，不会崩）
// ========================================================
static std::string bin_to_hex(const std::string& bin)
{
    // `bin` is LSB-first (index 0 is minterm 0) as produced by STP
    std::string b = bin;

    while (b.size() % 4 != 0)
        b = "0" + b;

    static const char* hex_map = "0123456789abcdef";
    std::string hex;
    hex.reserve(b.size() / 4);

    for (size_t i = 0; i < b.size(); i += 4)
    {
        int v = (b[i] - '0') * 8 +
                (b[i+1] - '0') * 4 +
                (b[i+2] - '0') * 2 +
                (b[i+3] - '0');
        hex.push_back(hex_map[v]);
    }

    while (hex.size() > 1 && hex[0] == '0')
        hex.erase(hex.begin());

    return hex;
}

// ========================================================
// write_bench
// ========================================================
class write_bench_command : public command
{
public:
    explicit write_bench_command(const environment::ptr& env)
        : command(env, "Write DSD result as LUT BENCH file")
    {
        add_option("file", filename,
            "output benchmark filename")->required();
    }

protected:
    void execute() override
    {
        if (filename.empty())
        {
            std::cout << "❌ No output file provided\n";
            return;
        }

        std::ofstream fout(filename);
        if (!fout)
        {
            std::cout << "❌ Cannot open " << filename << "\n";
            return;
        }

        if (NODE_LIST.empty())
        {
            std::cout << "❌ NODE_LIST is empty! (DSD not run?)\n";
            return;
        }

        if (ORIGINAL_VAR_COUNT == 0)
        {
            std::cout << "❌ ORIGINAL_VAR_COUNT is 0! (DSD not run?)\n";
            return;
        }

        // ====================================================
        // 1) 输入变量
        // ====================================================
        for (int v = 1; v <= ORIGINAL_VAR_COUNT; v++)
            fout << "INPUT(" << varname_from_id(v) << ")\n";

        fout << "OUTPUT(F0)\n\n";

        // ====================================================
        // 2) 节点命名
        // ====================================================
        std::map<int,std::string> name_of;
         
        
        for (auto &n : NODE_LIST)
        {
            if (n.func == "in")
                name_of[n.id] = varname_from_id(n.var_id);
        }

        for (auto &n : NODE_LIST)
        {
            if (n.func != "in")
                name_of[n.id] = "new_n" + std::to_string(n.id);
        }

        int root_id = NODE_LIST.back().id;
        name_of[root_id] = "F0";

        // ====================================================
        // 3) 输出 LUT（🔥 全部 child 顺序反转！）
        // ====================================================
        for (auto &n : NODE_LIST)
        {
            if (n.func == "in")
                continue;

            fout << name_of[n.id] << " = LUT 0x"
                 << bin_to_hex(n.func) << " (";

            // ⭐⭐⭐ 关键：反转 child 顺序，无条件统一
            std::vector<int> rev = n.child;
            std::reverse(rev.begin(), rev.end());

            for (size_t i = 0; i < rev.size(); i++)
            {
                fout << name_of[rev[i]];
                if (i + 1 < rev.size())
                    fout << ", ";
            }

            fout << ")\n";
        }

        // ====================================================
        // Done
        // ====================================================
        std::cout << "✅ BENCH written to " << filename << "\n\n";

        std::cout << "📋 变量映射（最低位→'a'）：\n";
        //for (int v = ORIGINAL_VAR_COUNT; v >= 1; v--)
        for (int v = 1; v <= ORIGINAL_VAR_COUNT; v++)
            std::cout << "   变量" << v << " → '" 
                      << varname_from_id(v) << "'\n";
    }

private:
    std::string filename{};
};

ALICE_ADD_COMMAND(write_bench, "STP")

} // namespace alice

#endif
