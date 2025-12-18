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
      // Keep 2-input and below LUTs unchanged as requested
    if ( children.size() <= 2u )
    {
      fn( ntk.create_node( children, function ) );
      return;
    }
      // Try STP bi-decomposition for 3+ input LUTs
    auto decomposition = stp::capture_bidecomposition( function );
    if ( !decomposition )
    {
           // Decomposition failed, keep original LUT
      fn( ntk.create_node( children, function ) );
      return;
    }

       // Map STP variable IDs to mockturtle signals
    // STP uses 1-based indexing: variables [1, 2, 3, ..., n]
    // In BENCH format, variables go left to right as LSB to MSB
    // mockturtle children[0] is LSB (variable 1), children[n-1] is MSB (variable n)
    std::unordered_map<int, typename Ntk::signal> var_to_signal;
    const auto n = children.size();
    
    for ( auto i = 0u; i < n; ++i )
    {
           // children[i] corresponds to STP variable (i + 1)
      // children[0] -> variable 1 (LSB)
      // children[n-1] -> variable n (MSB)
      int stp_var_id = static_cast<int>( i + 1 );
      var_to_signal[stp_var_id] = children[i];
    }

    // Build lookup table for DSD nodes
    std::unordered_map<int, DSDNode> node_lookup;
    for ( auto const& node : decomposition->nodes )
    {
           node_lookup[node.id] = node;
    }


    // Cache for built signals to avoid rebuilding same nodes
    std::unordered_map<int, typename Ntk::signal> cache;
      // Recursive function to build network from DSD nodes
    std::function<std::optional<typename Ntk::signal>( int )> build = 
         [&]( int node_id ) -> std::optional<typename Ntk::signal> 
    {
          // Check cache first
      auto cache_it = cache.find( node_id );
      if ( cache_it != cache.end() )
      {
             return cache_it->second;
      }
         // Find the node
      auto node_it = node_lookup.find( node_id );
      if ( node_it == node_lookup.end() )
      {
        return std::nullopt;
      }

      const auto& node = node_it->second;
         typename Ntk::signal result;

      // Handle input nodes
      if ( node.func == "in" )
      {
               auto var_it = var_to_signal.find( node.var_id );
        if ( var_it == var_to_signal.end() )
        {
          return std::nullopt;
        }
        result = var_it->second;
      }
      // Handle constant nodes
            else if ( node.func == "0" )
      {
        result = ntk.get_constant( false );
             }
      else if ( node.func == "1" )
      {
        result = ntk.get_constant( true );
           }
      // Handle internal nodes (gates/LUTs)
      else
      {
              // Build fanins recursively
        std::vector<typename Ntk::signal> fanins;
              fanins.reserve( node.child.size() );
        
        for ( auto child_id : node.child )
        {
                  auto child_signal = build( child_id );
          if ( !child_signal )
          {
            return std::nullopt;
          }
                   fanins.push_back( *child_signal );
        }
      // Create the node based on its function
        // node.func is a binary string representing the truth table
        // For n inputs, func has 2^n bits
        
        if ( node.func == "01" && fanins.size() == 1u )
        {
                 // Special case: inverter
          result = ntk.create_not( fanins[0] );
        }
        else
        {
                   // General case: create LUT with truth table
          const auto num_vars = node.child.size();
          kitty::dynamic_truth_table tt( num_vars );
          
          // node.func is stored with LSB first (index 0)
          // kitty expects bit 0 to be f(0,0,...,0)
          for ( size_t i = 0; i < node.func.size(); ++i )
          {
            if ( node.func[i] == '1' )
            {
              kitty::set_bit( tt, i );
            }
          }
          
          // STP stores children in the order they appear in node.child
          // write_bench reverses them for BENCH output, so we should do the same
          std::vector<typename Ntk::signal> reversed_fanins = fanins;
          std::reverse( reversed_fanins.begin(), reversed_fanins.end() );
                   result = ntk.create_node( reversed_fanins, tt );
        }
      }
     // Cache the result
      cache[node_id] = result;
      return result;
    };
   // Build from the root node
    auto root_signal = build( decomposition->root_id );
    if ( root_signal )
    {    fn( *root_signal );
    }
    else
    {     // Fallback: use original LUT if building failed
      fn( ntk.create_node( children, function ) );
    }
  }
};


}
#endif