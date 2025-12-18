#pragma once
#include <bits/stdc++.h>
using std::string;
using std::vector;
using std::set;
using std::cout;
using std::endl;


#include "excute.hpp"   // generate_swap_vec / Vec_chain_multiply / Vec_semi_tensor_product

bool run_dsd_recursive(const string& binary01);

//-----------------------------------------
// 判断是否为 2 的幂
//-----------------------------------------
static inline bool is_power_of_two(size_t x){
    return x && ((x & (x - 1)) == 0);
}

//-----------------------------------------
// 二进制串转为列向量
//-----------------------------------------
inline vector<stp_data> binary_to_vec(const string &bin)
{
    int rows = bin.size();
    vector<stp_data> M(rows + 1);
    M[0] = rows;
    for (int i = 0; i < rows; ++i)
        M[i + 1] = bin[i] - '0';
    return M;
}

//-----------------------------------------
// 块相关工具
//-----------------------------------------
static bool is_constant_block(const std::string& b){
    return std::all_of(b.begin(), b.end(), [&](char c){ return c==b.front(); });
}

static std::string complement_block(const std::string& b){
    std::string r=b;
    for(char& c: r) c = (c=='0' ? '1' : '0');
    return r;
}

//-----------------------------------------
// 定理 3.3 CASE 判断
//-----------------------------------------
static int theorem33_case_id(const std::string& binary, int s){
    const int n = std::log2(binary.size());
    if(!is_power_of_two(binary.size()) || (1<<n)!=(int)binary.size()) return 0;
    if(s<1 || s>n/2) return 0;

    const int block_len = 1<<s;
    const int num_blocks = binary.size() / block_len;

    std::vector<std::string> blocks;
    for(int i=0;i<num_blocks;++i)
        blocks.emplace_back(binary.substr(i*block_len, block_len));

    std::set<std::string> uniq_const, uniq_nonconst;
    for(auto &b: blocks)
        (is_constant_block(b) ? uniq_const : uniq_nonconst).insert(b);

    if(uniq_nonconst.empty() && uniq_const.size()==2) return 1;
    if(uniq_const.size()==1 && uniq_nonconst.size()==1) return 2;
    if(uniq_const.empty() && uniq_nonconst.size()==1) return 3;
    if(uniq_const.empty() && uniq_nonconst.size()==2){
        auto it = uniq_nonconst.begin();
        string a=*it++; string b=*it;
        if(complement_block(a)==b) return 4;
    }
    if(uniq_nonconst.empty() && uniq_const.size()==1) return 5;
    return 0;
}

// -----------------------------------------
//          重排主函数（核心功能）
// -----------------------------------------
inline void all_reorders(const string &binary)
{
    int len = binary.size();
    if(!is_power_of_two(len)){
        std::cout<<"输入长度必须是 2 的整数次幂\n";
        return;
    }
    int n = log2(len);
    int r = n / 2;

    vector<stp_data> Mf = binary_to_vec(binary);

    for(int s=1; s<=r; ++s)
    {
        vector<bool> v(n);
        fill(v.begin(), v.begin()+s, true);

        do{
            vector<int> Lambda;
            for(int i=0;i<n;i++)
                if(v[i]) Lambda.push_back(i+1);

            vector<vector<stp_data>> swap_chain;

            for(int k=s;k>=1;k--){
                int j_k = Lambda[k-1];
                int exp = j_k + (s-1) - k;
                swap_chain.push_back(generate_swap_vec(2, pow(2,exp)));
            }
            swap_chain.push_back(generate_swap_vec(pow(2,n-s), pow(2,s)));

            vector<stp_data> Mperm =
                Vec_chain_multiply(swap_chain,false);

            vector<stp_data> result =
                Vec_semi_tensor_product(Mf,Mperm);

            string reordered;
            for(size_t i=1;i<result.size();++i)
                reordered.push_back(result[i]?'1':'0');

            int cid = theorem33_case_id(reordered, s);
            if(cid!=0){
                cout << "\n===== 重排命中：s="<<s<<" 情形("<<cid<<") =====\n";
                cout << "Λ = { ";
                for(int j : Lambda) cout<<j<<" ";
                cout << "}  => reordered: " << reordered << "\n";

                // 🔥🔥🔥 关键：直接做分解
                run_dsd_recursive(reordered);

                // 🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥
                // 立即退出，不再继续重排//只要符合一个分解就直接退出
                // 🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥
                return;
            }

        }while(prev_permutation(v.begin(),v.end()));
    }

    cout << "❌ 所有重排均未命中任何分解模式\n";
}


// //////////////做所有的重排情况,上面的是若有一个命中就退出
// inline void all_reorders(const string &binary)
// {
//     int len = binary.size();
//     if(!is_power_of_two(len)){
//         std::cout<<"输入长度必须是 2 的整数次幂\n";
//         return;
//     }
//     int n = log2(len);
//     int r = n / 2;

//     vector<stp_data> Mf = binary_to_vec(binary);

//     for(int s=1; s<=r; ++s)
//     {
//         vector<bool> v(n);
//         fill(v.begin(), v.begin()+s, true);

//         do{
//             vector<int> Lambda;
//             for(int i=0;i<n;i++)
//                 if(v[i]) Lambda.push_back(i+1);

//             vector<vector<stp_data>> swap_chain;

//             for(int k=s;k>=1;k--){
//                 int j_k = Lambda[k-1];
//                 int exp = j_k + (s-1) - k;
//                 swap_chain.push_back(generate_swap_vec(2, pow(2,exp)));
//             }
//             swap_chain.push_back(generate_swap_vec(pow(2,n-s), pow(2,s)));

//             vector<stp_data> Mperm = Vec_chain_multiply(swap_chain,false);
//             vector<stp_data> result = Vec_semi_tensor_product(Mf,Mperm);

//             string reordered;
//             for(size_t i=1;i<result.size();++i)
//                 reordered.push_back(result[i]?'1':'0');

//             int cid = theorem33_case_id(reordered, s);
//             if(cid!=0){
//                 cout << "\n===== 重排命中：s="<<s<<" 情形("<<cid<<") =====\n";
//                 cout << "Λ = { ";
//                 for(int j : Lambda) cout<<j<<" ";
//                 cout << "}  => reordered: " << reordered << "\n";

//                 // 🔥🔥🔥 关键：继续做分解 !!! 🔥🔥🔥
//                 run_dsd_recursive(reordered, s);
//             }

//         }while(prev_permutation(v.begin(),v.end()));
//     }
// }

//==============================================================
//    含 x / 多字符重排专用：不做分解、不做模式分析
//==============================================================
inline void all_reorders_char(const string &raw)
{
    int len = raw.size();
    if(!is_power_of_two(len)){
        cout << "输入长度必须是 2 的整数次幂（支持任意字符）\n";
        return;
    }

    int n = log2(len);
    int r = n / 2;

    // 将任意字符按 ASCII 值存到 stp_data 中
    vector<stp_data> Mf(len + 1);
    Mf[0] = len;
    for(int i=0; i<len; ++i)
        Mf[i+1] = (unsigned char)raw[i];    // 保留字符，不做 0/1 转换

    for(int s = 1; s <= r; ++s)
    {
        vector<bool> v(n);
        fill(v.begin(), v.begin()+s, true);

        do{
            vector<int> Lambda;
            for(int i=0;i<n;i++)
                if(v[i]) Lambda.push_back(i+1);

            vector<vector<stp_data>> swap_chain;

            for(int k=s; k>=1; k--){
                int j_k = Lambda[k-1];
                int exp = j_k + (s-1) - k;
                swap_chain.push_back(generate_swap_vec(2, pow(2,exp)));
            }
            swap_chain.push_back(generate_swap_vec(pow(2,n-s), pow(2,s)));

            vector<stp_data> Mperm = Vec_chain_multiply(swap_chain,false);
            vector<stp_data> result = Vec_semi_tensor_product(Mf, Mperm);

            // 把 stp_data 还原为字符
            string reordered;
            reordered.reserve(len);
            for(int i=1; i<=len; ++i)
                reordered.push_back(char(result[i]));

            cout << "Λ = { ";
            for(int j : Lambda) cout << j << " ";
            cout << "}  => reordered: " << reordered << "\n";

        }while(prev_permutation(v.begin(), v.end()));
    }
}
