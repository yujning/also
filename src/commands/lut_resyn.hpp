/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file lut_resyn.hpp
 *
 * @brief TODO
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef LUT_RESYN_HPP
#define LUT_RESYN_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg3_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/shannon.hpp>
#include <mockturtle/algorithms/node_resynthesis/dsd.hpp>

#include "../networks/m5ig/m5ig.hpp"
#include "../networks/m5ig/m5ig_npn.hpp"
#include "../networks/m5ig/exact_online_m3ig.hpp"
#include "../networks/m5ig/exact_online_m5ig.hpp"
#include "../networks/img/img.hpp"
#include "../networks/img/img_npn.hpp"
#include "../networks/aoig/xag_lut_npn.hpp"
#include "../networks/aoig/xag_lut_dec.hpp"
#include "../networks/aoig/build_xag_db.hpp"
#include "../networks/img/img_all.hpp"
#include "../core/misc.hpp"
#include "../core/direct_mapping.hpp"


#include "../networks/aoig/stp_bidec_resynthesis.hpp"
namespace alice
{

  class lut_resyn_command: public command
  {
    public:
      explicit lut_resyn_command( const environment::ptr& env )
               : command( env, "lut resyn using optimal networks (default:mig)" )
      {
        add_option( "cut_size, -k", cut_size, "set the cut size from 2 to 8, default = 4" );
        add_flag( "--verbose, -v", "print the information" );
        add_flag( "--xmg, -x", "using xmg as target logic network" );
        add_flag( "--xmg3, -m", "using xmg3 as target logic network" );
        add_flag( "--test_m3ig", "using m3ig as target logic network, exact_m3ig online" );
        add_flag( "--test_m5ig", "using m5ig as target logic network, exact_m5ig online" );
        add_flag( "--m5ig, -r", "using m5ig as target logic network" );
        add_flag( "--img, -i",  "using img as target logic network" );
        add_flag( "--xag, -g",  "using xag as target logic network" );
        add_flag( "--new_entry, -n", "adds new store entry" );
        add_flag( "--enable_direct_mapping, -e", "enable aig to xmg by direct mapping for comparison" );
        add_flag("--stp_bd, -d","use bi_decomposition to run stp for dsd ") ;
        add_flag( "--aa", "convert klut to xag graph (no resynthesis)" );
        add_flag( "--mm", "convert klut to xmg graph (no resynthesis)" );
      }

      rules validity_rules() const
      {
        return { has_store_element<klut_network>( env ) };
      }


protected:
  void execute()
  {
    /* ============================================================
     * 1. get current klut
     * ============================================================ */
    klut_network cur_klut = store<klut_network>().current();

    /* ============================================================
     * 2. stp-based bi-decomposition (-d)
     *    pre-processing stage
     * ============================================================ */
    if ( is_set( "stp_bd" ) )
    {
      also::stp_bidec_lut_resynthesis<klut_network> resyn;
      cur_klut = node_resynthesis<klut_network>( cur_klut, resyn );

      if ( is_set( "new_entry" ) )
      {
        store<klut_network>().extend();
        store<klut_network>().current() = cleanup_dangling( cur_klut );
      }
    }

    /* ============================================================
     * 3. convert klut -> xag (--gg)
     *    NOTE: NO resynthesis
     * ============================================================ */
    if ( is_set( "aa" ) )
    {
      xag_network xag = convert_klut_to_graph<xag_network>( cur_klut );

      if ( is_set( "new_entry" ) )
      {
        store<xag_network>().extend();
        store<xag_network>().current() = cleanup_dangling( xag );
      }
      return;
    }

    if ( is_set( "mm" ) )
    {
      xmg_network xmg = convert_klut_to_graph<xmg_network>( cur_klut );

      if ( is_set( "new_entry" ) )
      {
        store<xmg_network>().extend();
        store<xmg_network>().current() = cleanup_dangling( xmg );
      }
      return;
    }

    /* ============================================================
     * 4. original target network selection (unchanged semantics)
     * ============================================================ */

    if ( is_set( "xmg" ) )
    {
      xmg_network xmg;

      if ( is_set( "enable_direct_mapping" ) )
      
      {
        assert( store<aig_network>().size() > 0 );
        aig_network aig = store<aig_network>().current();
        xmg = also::xmg_from_aig( aig );
      }
      // else 
      // {
      //   xmg_npn_resynthesis resyn;
      //   xmg = node_resynthesis<xmg_network>( cur_klut, resyn );
      // }
      else
      {
         xmg = convert_klut_to_graph<xmg_network>( cur_klut );

      }

      if ( is_set( "new_entry" ) )
      {
        store<xmg_network>().extend();
        store<xmg_network>().current() = cleanup_dangling( xmg );
      }
    }
    else if ( is_set( "xmg3" ) )
    {
      xmg_network xmg;
      xmg3_npn_resynthesis<xmg_network> resyn;
      xmg = node_resynthesis<xmg_network>( cur_klut, resyn );

      if ( is_set( "new_entry" ) )
      {
        store<xmg_network>().extend();
        store<xmg_network>().current() = xmg;
      }
    }
    else if ( is_set( "m5ig" ) )
    {
      m5ig_network m5ig;

      if ( cut_size <= 4 )
      {
        m5ig_npn_resynthesis resyn;
        m5ig = node_resynthesis<m5ig_network>( cur_klut, resyn );
      }

      if ( is_set( "new_entry" ) )
      {
        store<m5ig_network>().extend();
        store<m5ig_network>().current() = m5ig;
      }
    }
    else if ( is_set( "img" ) )
    {
      img_network img;
      img_all_resynthesis<img_network> resyn;
      img = node_resynthesis<img_network>( cur_klut, resyn );

      if ( is_set( "new_entry" ) )
      {
        store<img_network>().extend();
        store<img_network>().current() = img;
      }
    }
    else if ( is_set( "xag" ) )
    {
      xag_network xag;

      if ( cut_size <= 4 )
      {
        xag_npn_lut_resynthesis resyn;
        xag = node_resynthesis<xag_network>( cur_klut, resyn );
      }
      else
      {
        xag_network db;
        auto opt_xags = also::load_xag_string_db( db );
        xag_lut_dec_resynthesis<xag_network> resyn( opt_xags );
        xag = node_resynthesis<xag_network>( cur_klut, resyn );
      }

      if ( is_set( "new_entry" ) )
      {
        store<xag_network>().extend();
        store<xag_network>().current() = xag;
      }
    }
    else
    {
      mig_npn_resynthesis resyn;
      auto mig = node_resynthesis<mig_network>( cur_klut, resyn );

      if ( is_set( "new_entry" ) )
      {
        store<mig_network>().extend();
        store<mig_network>().current() = mig;
      }
    }
  }


    private:
      int cut_size = 8;
  };

  ALICE_ADD_COMMAND( lut_resyn, "Exact synthesis" )


}

#endif
