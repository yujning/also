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

    std::vector<int> original_from_var_id( decomposition->variable_order.size() + 1, 0 );
    for ( auto index = 0u; index < decomposition->variable_order.size(); ++index )
    {
      const auto original_var = decomposition->variable_order[index];
      const auto mapped_var_id = static_cast<int>( index + 1 );

      if ( original_var <= 0 )
      {
        return;
      }
          original_from_var_id[mapped_var_id] = original_var;
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
      if ( node.var_id <= 0 || node.var_id >= static_cast<int>( original_from_var_id.size() ) )
        {
          return std::nullopt;
        }
               const auto original_var = original_from_var_id[node.var_id];
        if ( original_var <= 0 || static_cast<size_t>( original_var ) > children.size() )
        {
          return std::nullopt;
        }

        result = children[original_var - 1];
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