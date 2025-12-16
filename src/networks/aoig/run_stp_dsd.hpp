#ifndef RUN_STP_DSD_HPP
#define RUN_STP_DSD_HPP

#include <string>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <lorina/bench.hpp>
#include <mockturtle/io/bench_reader.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/algorithms/klut_to_graph.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <kitty/kitty.hpp>


namespace also {

/*───────────────────────────────────────────────*
 * 工具函数：写入字符串到文件
 *───────────────────────────────────────────────*/
inline void write_string_to_file( const std::string& path, const std::string& content )
{
    std::ofstream ofs(path);
    ofs << content;
}

/*───────────────────────────────────────────────*
 * 运行 STP 并返回 bench 文件路径
 *───────────────────────────────────────────────*/
inline std::string run_stp_dsd( const std::string& tt_hex )
{
    const char* env_bench_path = std::getenv( "BENCH_FILE_PATH" );
    std::filesystem::path bench_path = env_bench_path != nullptr ? env_bench_path
    : std::filesystem::temp_directory_path() / "stp_dsd_out.bench";

        /* 清理旧文件，保证输出是本次运行生成的 */
        if ( std::filesystem::exists( bench_path ) )
        {
            std::filesystem::remove( bench_path );
        }

    const char* env_stp_bin = std::getenv( "STP_BIN_PATH" );
    std::string stp_bin = env_stp_bin != nullptr ? env_stp_bin
                                                 : "/home/yjn/stp10_29/stp/build/bin/stp";

    /* 生成临时命令脚本 */
    std::filesystem::path cmd_path = std::filesystem::temp_directory_path() / "stp_input_cmds.txt";
    {
        std::ofstream ofs( cmd_path );
        ofs << "dsd -f " << tt_hex << "\n";
        ofs << "write_bench " << bench_path.string() << "\n";
        ofs << "quit\n";   // 保险退出
    }

    /* 调用你的交互式 stp：通过 stdin 输入命令脚本 */
    std::string cmd = stp_bin + " < " + cmd_path.string();

    int ret = std::system( cmd.c_str() );
    if ( ret != 0 )
    {
        std::cerr << "[stp_dsd] command failed\n";
        return "";
    }

    if ( !std::filesystem::exists( bench_path ) )
    {
        std::cerr << "[stp_dsd] bench file not generated\n";
        return "";
    }

    return bench_path;
}


/*───────────────────────────────────────────────*
 * 顶层：运行 STP → bench → klut → xag → merge
 *───────────────────────────────────────────────*/
inline mockturtle::xag_network::signal
stp_dsd( mockturtle::xag_network& _ntk,
         kitty::dynamic_truth_table const& remainder,
         std::vector<mockturtle::xag_network::signal> const& children )
{
    /* ① 转 TT 为 0x.. 字符串 */
    std::string tt_hex = "0x" + kitty::to_hex( remainder );
    std ::cout<<"remainder: "<<tt_hex<<"\n";

    /* ② 调用 STP 生成 bench */
    std::string bench_file = run_stp_dsd( tt_hex );
    if ( bench_file.empty() )
        return {}; // signal{} 表示失败

    /* ③ bench → klut */
    mockturtle::klut_network klut;
    auto result = lorina::read_bench( bench_file, mockturtle::bench_reader( klut ) );
    if ( result != lorina::return_code::success )
        return {};

    /* ④ KLUT → XAG 临时网络 */
    mockturtle::xag_network tmp_xag =
        mockturtle::convert_klut_to_graph<mockturtle::xag_network>( klut );

    if ( tmp_xag.num_gates() == 0 )
        return {}; // nothing to merge

    /* ⑤ 将 tmp_xag 合并到 _ntk */
    mockturtle::node_map<mockturtle::xag_network::signal, mockturtle::xag_network> old2new( tmp_xag );

    /* 常量映射 */
    old2new[tmp_xag.get_node( tmp_xag.get_constant( false ) )] = _ntk.get_constant( false );
    old2new[tmp_xag.get_node( tmp_xag.get_constant( true ) )] = _ntk.get_constant( true );

    /* 主输入映射到现有 children */
    if ( tmp_xag.num_pis() > children.size() )
    {
        return {};
    }

    tmp_xag.foreach_pi( [&]( auto n, auto index ) {
        old2new[n] = children[index];
    } );

    /* 复制门 */
    tmp_xag.foreach_gate( [&]( auto n ) {
        std::vector<mockturtle::xag_network::signal> fanins;
        tmp_xag.foreach_fanin( n, [&]( auto const& f ) {
            auto mapped = old2new[tmp_xag.get_node( f )];
            if ( tmp_xag.is_complemented( f ) )
            {
                mapped = _ntk.create_not( mapped );
            }
            fanins.push_back( mapped );
        } );

        mockturtle::xag_network::signal new_sig{};
        if ( tmp_xag.is_and( n ) )
        {
            new_sig = _ntk.create_and( fanins[0], fanins[1] );
        }
        else
        {
            new_sig = _ntk.create_xor( fanins[0], fanins[1] );
        }

        old2new[n] = new_sig;
    } );

    /* 取第一个 PO 作为返回值 */
    mockturtle::xag_network::signal out_sig{};
    tmp_xag.foreach_po( [&]( auto const& f, auto index ) {
        if ( index == 0u )
        {
              out_sig = old2new[tmp_xag.get_node( f )];
            if ( tmp_xag.is_complemented( f ) )
            {
                out_sig = _ntk.create_not( out_sig );
            }
        }
    } );

    return out_sig; /* 返回 STP 获得的顶层 XAG 子图 */
}

} // namespace also

#endif
