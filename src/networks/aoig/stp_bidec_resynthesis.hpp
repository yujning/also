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
#include <mockturtle/networks/klut.hpp>

#include <algorithm>

#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

namespace also
{

template<class Ntk>
class stp_bidec_lut_resynthesis
{
public:
  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
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
      return;
    }

    
    if ( decomposition->variable_order.size() > children.size() )
    {
      return;
    }

        std::unordered_map<int, typename Ntk::signal> var_to_signal;
    if ( decomposition->variable_order.size() == children.size() )
    {
            for ( auto i = 0u; i < decomposition->variable_order.size(); ++i )
      {
        var_to_signal.emplace( decomposition->variable_order[i], children[i] );
      }
    }
    else
    {
      for ( auto i = 0u; i < children.size(); ++i )
      {
        var_to_signal.emplace( static_cast<int>( i + 1 ), children[i] );
      }
    }


    std::unordered_map<int, DSDNode> node_lookup;
    for ( auto const& node : decomposition->nodes )
    {
       node_lookup.try_emplace( node.id, node );
    }

    std::unordered_map<int, typename Ntk::signal> cache;

    std::function<std::optional<typename Ntk::signal>( int )> build = [&]( int id ) -> std::optional<typename Ntk::signal> {
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

      cache.emplace( id, result );
      return result;
    };

    if ( auto root = build( decomposition->root_id ) )
    {
      fn( *root );
    }
  }
};

} // namespace also

#endif