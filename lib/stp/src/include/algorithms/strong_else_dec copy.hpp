#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

// globals / node creation
#include "node_global.hpp"

// kitty + mockturtle exact
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/constructors.hpp>
#include <kitty/print.hpp>
#include <limits>
#include <kitty/operations.hpp>
// forward declaration
inline std::vector<int>
make_children_from_order_with_placeholder(
    const std::vector<int>& order,
    const std::unordered_map<int, int>* placeholder_nodes,
    const std::vector<int>* local_to_global);



// ---------- helper: resolve a var_id to a node_id (placeholder-aware) ----------
inline int strong_resolve_var_node_id(
    int var_id,
    const std::vector<int>* local_to_global,
    const std::unordered_map<int,int>* placeholder_nodes )
{
  // 1) local placeholder
  if ( placeholder_nodes )
  {
    auto it = placeholder_nodes->find( var_id );
    if ( it != placeholder_nodes->end() )
      return it->second;
  }

  // 2) global placeholder bindings
  auto git = PLACEHOLDER_BINDINGS.find( var_id );
  if ( git != PLACEHOLDER_BINDINGS.end() )
    return git->second;

  // 3) local_to_global mapping
  int global_var = var_id;
  if ( local_to_global &&
       var_id >= 0 &&
       var_id < (int)local_to_global->size() &&
       (*local_to_global)[var_id] != 0 )
  {
    global_var = (*local_to_global)[var_id];
  }

  // 4) input node
  return new_in_node( global_var );
}

// ---------- helper: build children in kitty var order (LSB->MSB) ----------
inline std::vector<int> strong_make_children_kitty_order(
    const std::vector<int>& order_msb2lsb,
    const std::vector<int>* local_to_global,
    const std::unordered_map<int,int>* placeholder_nodes )
{
  std::vector<int> ch;
  ch.reserve( order_msb2lsb.size() );

  // kitty var0 = LSB, so bind from order.back() to order.front()
  for ( auto it = order_msb2lsb.rbegin(); it != order_msb2lsb.rend(); ++it )
  {
    ch.push_back( strong_resolve_var_node_id( *it, local_to_global, placeholder_nodes ) );
  }
  return ch;
}

// =====================================================
// exact refine (2-LUT) but placeholder-aware
// =====================================================
inline int strong_else_decompose(
    const std::string& mf,
    const std::vector<int>& order,
    int depth,
    const std::vector<int>* local_to_global,
       const std::unordered_map<int, int>* placeholder_nodes,

    // callback to Strong recursion
    int (*strong_rec)(
        const std::string&,
        const std::vector<int>&,
        int,
        const std::vector<int>*,
        const std::unordered_map<int, int>*))
{
  const int n = (int)order.size();

  std::string indent((size_t)depth * 2, ' ');
  std::cout << indent << "⚠️ Strong EXACT refine (n=" << n << ")\n";

  if ( n == 0 )
  {
    return new_node( mf, {} );
  }

  std::cout << indent << "⚠️ Strong fallback: Shannon (n=" << n << ")\n";

  for ( char c : mf )
  {
    if ( c != '0' && c != '1' )
{
      throw std::runtime_error( "strong_else_decompose: mf contains non-binary char" );
    }
  }
  kitty::dynamic_truth_table tt( n );
  kitty::create_from_binary_string( tt, mf );
  std::vector<uint32_t> support;
  support.reserve( n );
  for ( uint32_t i = 0; i < static_cast<uint32_t>( n ); ++i )
  {
    if ( kitty::has_var( tt, i ) )
    {
      support.push_back( i );
    }
  }

  if ( support.empty() )
  {
    return new_node( mf, {} );
  }

 uint32_t var_min = support.front();
  int best_score = std::numeric_limits<int>::max();
  for ( auto var : support )
  {
    const auto co00 = kitty::cofactor0( tt, var );
    const auto co11 = kitty::cofactor1( tt, var );
    int s_co0 = 0;
    int s_co1 = 0;
    for ( uint32_t s_sd = 0; s_sd < co00.num_vars(); ++s_sd )
    {
      if ( kitty::has_var( co00, s_sd ) )
      {
        s_co0 += 1;
      }
    }
    for ( uint32_t s_sd = 0; s_sd < co11.num_vars(); ++s_sd )
    {
      if ( kitty::has_var( co11, s_sd ) )
      {
        s_co1 += 1;
      }
    }
    const int add = s_co0 + s_co1;
    if ( add < best_score )
    {
      best_score = add;
      var_min = var;
    }
  }

    const size_t order_index = static_cast<size_t>( n - 1u - var_min );
  auto children = make_children_from_order_with_placeholder(
      order, placeholder_nodes, local_to_global );
  const int pivot = children.at( order_index );

  auto build_cofactor_tt = [&]( bool value ) {
    const uint32_t new_vars = static_cast<uint32_t>( n - 1 );
    kitty::dynamic_truth_table new_tt( new_vars );
    const uint64_t limit = 1ull << new_vars;

// =====================================================
// Strong DSD fallback入口
// =====================================================
for ( uint64_t x = 0; x < limit; ++x )
    {
      uint64_t old = 0;
      for ( uint32_t b = 0; b < new_vars; ++b )
      {
        if ( x & ( 1ull << b ) )
        {
          const uint32_t old_pos = b < var_min ? b : b + 1u;
          old |= ( 1ull << old_pos );
        }
      }
      if ( value )
      {
        old |= ( 1ull << var_min );
      }


      if ( kitty::get_bit( tt, old ) )
      {
        kitty::set_bit( new_tt, x );
      }
    }

    std::vector<int> out_order = order;
    if ( !out_order.empty() )
    {
      out_order.erase( out_order.begin() + static_cast<long>( order_index ) );
    }

    return std::pair<std::string, std::vector<int>>{
        kitty::to_binary( new_tt ),
        std::move( out_order ) };
  };

  auto [f_pos, pos_order] = build_cofactor_tt( false );
  auto [f_neg, neg_order] = build_cofactor_tt( true );

  const int pos_node =
     strong_rec( f_pos, pos_order, depth + 1, local_to_global, placeholder_nodes );

  const int neg_node =
    strong_rec( f_neg, neg_order, depth + 1, local_to_global, placeholder_nodes );

  const int pos_term = new_node( "1000", { pivot, pos_node } );
  const int neg_term = new_node( "0010", { pivot, neg_node } );

  return new_node( "1110", { pos_term, neg_term } );
}
