/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file sdc.hpp
 *
 * @brief Compute satisfiability don't cares for a network
 *
 * @author Your Name
 * @since  0.1
 */

#ifndef SDC_HPP
#define SDC_HPP

#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/dont_cares.hpp>
#include <mockturtle/views/topo_view.hpp>

namespace alice
{

class sdc_command : public command
{
  public:
    explicit sdc_command( const environment::ptr& env ) : command( env, "Compute satisfiability don't cares (SDC)" )
    {
      add_flag("--verbose", "verbose output");
      add_flag("--lut, -l", "using LUT network");
      add_flag("--aig, -a", "using AIG network");
      add_option("--max_inputs,-m", max_inputs, "maximum number of TFI inputs (default: 16)", true);
    }

  protected:
    void execute()
    {
      try {
        if (is_set("lut"))
        {
          klut_network klut = store<klut_network>().current();
          compute_sdc(klut);
        }
        else if (is_set("aig"))
        {
          aig_network aig = store<aig_network>().current();
          compute_sdc(aig);
        }
        else
        {
          env->out() << "Error: please specify either --lut or --aig\n";
        }
      }
      catch (const std::exception& e)
      {
        env->err() << "Error computing SDC: " << e.what() << "\n";
      }
    }

    template<typename Ntk>
    void compute_sdc(Ntk const& ntk)
    {
      // Create a topological view to ensure proper node ordering
      mockturtle::topo_view topo_ntk{ntk};

      // Get the leaves (primary inputs) of the network
      std::vector<typename Ntk::node> leaves;
      topo_ntk.foreach_pi([&](auto const& node) {
        leaves.push_back(node);
      });

      if (leaves.empty())
      {
        env->err() << "Error: Network has no primary inputs\n";
        return;
      }

      if (is_set("verbose"))
      {
        env->out() << "Computing SDC for network with " << leaves.size() << " primary inputs\n";
        env->out() << "Maximum TFI inputs: " << max_inputs << "\n";
      }

      // Compute the satisfiability don't cares using mockturtle's implementation
      auto care = mockturtle::satisfiability_dont_cares(topo_ntk, leaves, max_inputs);

      // Output the result
      env->out() << "Satisfiability don't cares (SDC): " << kitty::to_hex(care) << "\n";

      if (is_set("verbose"))
      {
        env->out() << "Detailed information:\n";
        env->out() << "  Number of care set entries: " << kitty::count_ones(care) << "\n";
        env->out() << "  Number of don't care entries: " << ((1 << leaves.size()) - kitty::count_ones(care)) << "\n";
      }
    }

  private:
    uint64_t max_inputs = 16u;
};

ALICE_ADD_COMMAND(sdc, "Verification")

} // namespace alice

#endif