#ifndef RUN_STP_DSD_HPP
#define RUN_STP_DSD_HPP

#include <string>
#include <cstdlib>
#include <fstream>
#include <filesystem>

#include <lorina/bench.hpp>
#include <mockturtle/io/bench_reader.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/algorithms/klut_to_graph.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/algorithms/klut_to_graph.hpp>
#include <mockturtle/networks/utils.hpp>   // clone 在这里
#include <mockturtle/traits.hpp>           // node_map 在这里


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
    std::string bench_path = "/tmp/stp_dsd_out.bench";

    std::string cmd =
        "/home/yjn/stp10_29/stp/build/bin/stp "
        "dsd -f " + tt_hex +
        " ; "
        "/home/yjn/stp10_29/stp/build/bin/stp "
        "write_bench " + bench_path;

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
        template<typename NtkSrc, typename NtkDst, typename NodeMap>
        signal<NtkDst> clone( NtkSrc const& src, NtkDst& dst, NodeMap& old2new );


    return out_sig; /* 返回 STP 获得的顶层 XAG 子图 */
}

} // namespace also

#endif
