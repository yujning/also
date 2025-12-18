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
  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, 
                  LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    std::vector<typename Ntk::signal> children( begin, end );
    
    if ( children.size() <= 2u )
    {
      fn( ntk.create_node( children, function ) );
      return;
    }

    auto decomposition = stp::capture_bidecomposition( function );
    if ( !decomposition )
    {
      fn( ntk.create_node( children, function ) );
      return;
    }

    // ⭐ 关键改动1：创建变量上下文的唯一标识
    std::vector<uint64_t> context_key;
    context_key.reserve( children.size() );
    for ( auto const& sig : children )
    {
      context_key.push_back( ntk.node_to_index( ntk.get_node( sig ) ) );
    }
    
    // ⭐ 关键改动2：将上下文哈希纳入 cache key
    struct CacheKey 
    {
      int dsd_node_id;
      std::vector<uint64_t> var_context;
      
      bool operator==( CacheKey const& other ) const 
      {
        return dsd_node_id == other.dsd_node_id && 
               var_context == other.var_context;
      }
    };
    
    struct CacheKeyHash 
    {
      std::size_t operator()( CacheKey const& k ) const 
      {
        std::size_t seed = std::hash<int>{}( k.dsd_node_id );
        for ( auto v : k.var_context )
        {
          seed ^= std::hash<uint64_t>{}( v ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
      }
    };

    // 变量映射
    std::unordered_map<int, typename Ntk::signal> var_to_signal;
    const auto n = children.size();
    for ( auto i = 0u; i < n; ++i )
    {
      var_to_signal.emplace( static_cast<int>( n - i ), children[i] );
    }

    std::unordered_map<int, DSDNode> node_lookup;
    for ( auto const& node : decomposition->nodes )
    {
      node_lookup.try_emplace( node.id, node );
    }

    // ⭐ 关键改动3：使用新的 cache key 类型
    std::unordered_map<CacheKey, typename Ntk::signal, CacheKeyHash> cache;

    std::function<std::optional<typename Ntk::signal>( int )> build = 
      [&]( int id ) -> std::optional<typename Ntk::signal> 
    {
      CacheKey key{ id, context_key };
      
      if ( auto it = cache.find( key ); it != cache.end() )
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
        if ( auto it = var_to_signal.find( node.var_id ); it != var_to_signal.end() )
        {
          result = it->second;
        }
        else
        {
          return std::nullopt;
        }
      }
      else if ( node.func == "0" )
      {
        result = ntk.get_constant( false );
      }
      else if ( node.func == "1" )
      {
        result = ntk.get_constant( true );
      }
      else
      {
        std::vector<int> child_ids = node.child;
        std::reverse( child_ids.begin(), child_ids.end() );

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

        if ( node.func == "01" && fanins.size() == 1u )
        {
          result = ntk.create_not( fanins.front() );
        }
        else
        {
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
        }
      }

      cache.emplace( key, result );
      return result;
    };

    if ( auto root = build( decomposition->root_id ) )
    {
      fn( *root );
    }
    else
    {
      fn( ntk.create_node( children, function ) );
    }
  }
};

} // namespace also

#endif