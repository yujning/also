/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file imgff.hpp
 *
 * @brief construct a fanout-free implication logic network
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef LC_HPP
#define LC_HPP

#include <mockturtle/algorithms/simulation.hpp>
#include <chrono>
namespace alice
{

  class lc_command : public command
  {
    public:
      explicit lc_command( const environment::ptr& env ) : command( env, "lc" )
      {
        add_flag( "--verbose", "verbose output" );
        add_flag( "--lut, -l",  "using lutc network" );
        add_flag( "--aig, -a",  "using aig network" );
      }
      

      protected:
      void execute()
      {
        auto start = std::chrono::high_resolution_clock::now();
        if( is_set( "lut" ) )
        {
            klut_network klut = store<klut_network>().current();
            default_simulator<kitty::dynamic_truth_table> sim( klut.num_pis() );
            const auto tts = simulate_nodes<kitty::dynamic_truth_table>( klut, sim );
             klut.foreach_node( [&]( auto const& n, auto i ) {
             std::cout << fmt::format( "truth table of node {} is {}\n", i, kitty::to_hex( tts[n] ) );
         });
        }
        if( is_set( "aig" ) )
        {
            aig_network aig = store<aig_network>().current();
            default_simulator<kitty::dynamic_truth_table> sim( aig.num_pis() );
            const auto tts = simulate_nodes<kitty::dynamic_truth_table>( aig, sim );

            //aig.foreach_node( [&]( auto const& n, auto i ) {
             //std::cout << fmt::format( "truth table of node {} is {}\n", i, kitty::to_hex( tts[n] ) );
        // });
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << " time: " << time << " us\n"<< std::endl;
      }
    
    private:
  };

  ALICE_ADD_COMMAND( lc, "Rewriting" )


}

#endif
