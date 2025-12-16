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

namespace also
{

template<class Ntk>
class stp_bidec_lut_resynthesis
{
public:
  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    auto decomposition = stp::capture_bidecomposition( function );
    if ( !decomposition )
    {
      return;
    }

    std::vector<typename Ntk::signal> children( begin, end );
    if ( decomposition->variable_order.size() > children.size() )
    {
      return;
    }

    std::unordered_map<int, typename Ntk::signal> inputs;
    for ( auto index = 0u; index < decomposition->variable_order.size(); ++index )
    {
      const auto var_id = decomposition->variable_order[index];
       if ( var_id <= 0 || index >= children.size() )
      {
        return;
      }
         inputs.emplace( var_id, children[index] );
    }

    std::unordered_map<int, DSDNode> node_lookup;
    for ( auto const& node : decomposition->nodes )
    {
      node_lookup.emplace( node.id, node );
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
        auto input_it = inputs.find( node.var_id );
        if ( input_it == inputs.end() )
        {
          return std::nullopt;
        }
        result = input_it->second;
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
              kitty::set_bit( tt, i );
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