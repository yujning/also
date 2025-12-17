/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stp_bidec_resynthesis.hpp
 *
 * @brief Resynthesize KLUTs using STP bi-decomposition into 2-LUT structures.
 */

#ifndef STP_BIDEC_RESYNTHESIS_HPP
#define STP_BIDEC_RESYNTHESIS_HPP

#include <api/truth_table.hpp>

#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/topo_view.hpp>

#include <algorithm>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace also
{

template<class Ntk>
class stp_bidec_lut_resynthesis
{
public:
  // 记录分解信息的结构
  struct decomp_info
  {
    int original_node_id;
    std::string original_node_name;
    std::vector<int> sub_node_ids;
    std::vector<std::string> sub_node_names;
  };

  std::vector<decomp_info> decomposition_log;

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, 
                  LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    std::vector<typename Ntk::signal> children( begin, end );
    
    // 对于2输入及以下，直接创建节点
    if ( children.size() <= 2u )
    {
      fn( ntk.create_node( children, function ) );
      return;
    }

    // 调用STP双分解
    auto decomposition = stp::capture_bidecomposition( function );
    if ( !decomposition )
    {
      // 分解失败，使用原始LUT
      std::cout << "⚠️  STP分解失败，保持原始LUT\n";
      fn( ntk.create_node( children, function ) );
      return;
    }

    if ( decomposition->variable_order.size() > children.size() )
    {
      std::cout << "⚠️  变量顺序不匹配，保持原始LUT\n";
      fn( ntk.create_node( children, function ) );
      return;
    }

    std::cout << "\n📌 开始分解LUT (输入数=" << children.size() << ")\n";

    // ⭐⭐⭐ 关键：反转映射，匹配bd的变量编号约定
    // children[0] 是最低位 → 对应 bd 中的变量 n
    // children[n-1] 是最高位 → 对应 bd 中的变量 1
    std::unordered_map<int, typename Ntk::signal> var_to_signal;
    const auto n = children.size();
    for ( auto i = 0u; i < n; ++i )
    {
      var_to_signal.emplace( static_cast<int>( n - i ), children[i] );
    }

    // 构建节点查找表
    std::unordered_map<int, DSDNode> node_lookup;
    for ( auto const& node : decomposition->nodes )
    {
      node_lookup.try_emplace( node.id, node );
    }

    // 缓存已构建的节点
    std::unordered_map<int, typename Ntk::signal> cache;
    
    // 记录创建的子节点（用于命名）
    std::vector<typename Ntk::signal> created_nodes;

    // 递归构建函数
    std::function<std::optional<typename Ntk::signal>( int )> build = 
      [&]( int id ) -> std::optional<typename Ntk::signal> 
    {
      if ( auto it = cache.find( id ); it != cache.end() )
      {
        return it->second;
      }

      auto node_it = node_lookup.find( id );
      if ( node_it == node_lookup.end() )
      {
        return std::nullopt;
      }

      const auto& node = node_it->second;
      typename Ntk::signal result{};

      if ( node.func == "in" )
      {
        // 输入节点
        if ( auto it = var_to_signal.find( node.var_id ); it != var_to_signal.end() )
        {
          result = it->second;
          std::cout << "  输入节点 " << id << " → 变量 " << node.var_id << "\n";
        }
        else
        {
          return std::nullopt;
        }
      }
      else if ( node.func == "0" )
      {
        result = ntk.get_constant( false );
        std::cout << "  常数0节点 " << id << "\n";
      }
      else if ( node.func == "1" )
      {
        result = ntk.get_constant( true );
        std::cout << "  常数1节点 " << id << "\n";
      }
      else
      {
        // 内部节点
        std::vector<int> child_ids = node.child;
        
        // ⭐⭐⭐ 关键：反转子节点顺序，匹配mockturtle约定
        std::reverse(child_ids.begin(), child_ids.end());

        std::vector<typename Ntk::signal> fanins;
        fanins.reserve( child_ids.size() );
        for ( auto child_id : child_ids )
        {
          auto child_sig = build( child_id );
          if ( !child_sig )
          {
            return std::nullopt;
          }
          fanins.push_back( *child_sig );
        }

        // 特殊处理NOT门
        if ( node.func == "01" && fanins.size() == 1u )
        {
          result = ntk.create_not( fanins.front() );
          std::cout << "  NOT节点 " << id << " (func=" << node.func << ")\n";
        }
        else
        {
          // 创建LUT节点
          kitty::dynamic_truth_table tt( node.child.size() );
          for ( auto i = 0u; i < node.func.size(); ++i )
          {
            if ( node.func[i] == '1' )
            {
              const auto bit_index = node.func.size() - 1 - i;
              kitty::set_bit( tt, bit_index );
            }
          }
          result = ntk.create_node( fanins, tt );
          created_nodes.push_back( result );
          
          std::cout << "  LUT节点 " << id << " (func=" << node.func 
                    << ", 输入数=" << fanins.size() << ")\n";
        }
      }

      cache.emplace( id, result );
      return result;
    };

    // 构建根节点
    if ( auto root = build( decomposition->root_id ) )
    {
      std::cout << "✅ 分解完成，共创建 " << created_nodes.size() << " 个子节点\n";
      fn( *root );
    }
    else
    {
      std::cout << "❌ 分解构建失败，使用原始LUT\n";
      fn( ntk.create_node( children, function ) );
    }
  }
};

} // namespace also

#endif