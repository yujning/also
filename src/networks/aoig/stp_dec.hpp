#ifndef STP_DEC_HPP
#define STP_DEC_HPP

#include "../../core/misc.hpp"
#include "build_xag_db.hpp"
#include "run_stp_dsd.hpp"
#include "xag_dec.hpp"   // for xag_dec_params


#include <sstream>
#include <string>
#include <filesystem>
#include <fstream>
#include <mockturtle/io/bench_reader.hpp>
#include <mockturtle/networks/klut.hpp>
#include <lorina/bench.hpp>
#include <mockturtle/algorithms/klut_to_graph.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <kitty/constructors.hpp>


namespace also {

/*─────────────────────────────────────────────────────────*
 *  STP 顶层分解实现类
 *─────────────────────────────────────────────────────────*/
 class stp_dec_impl
  {
    private:
      std::vector<uint8_t> get_func_supports( kitty::dynamic_truth_table const& spec )
      {
        std::vector<uint8_t> supports;

        for( auto i = 0u; i < spec.num_vars(); ++i )
        {
          if( kitty::has_var( spec, i ) )
          {
            supports.push_back( i );
          }
        }

        return supports;
      }

    public:
      stp_dec_impl( xag_network& ntk, kitty::dynamic_truth_table const& func, std::vector<xag_network::signal> const& children, 
                    std::unordered_map<std::string, std::string>& opt_xags, 
                    xag_dec_params const& ps )
        : _ntk( ntk ),
          _func( func ),
          pis( children ),
          opt_xags( opt_xags ),
          _ps( ps )
      {
      }

      xag_network::signal decompose( kitty::dynamic_truth_table& remainder, std::vector<xag_network::signal> const& children )
      {
        auto sup = get_func_supports( remainder );
        
        /* check constants */
        if ( kitty::is_const0( remainder ) )
        {
          return _ntk.get_constant( false );
        }
        if ( kitty::is_const0( ~remainder ) )
        {
          return _ntk.get_constant( true );
        }
        
        /* check primary inputs */
        if( sup.size() == 1u )
        {
          auto var = remainder.construct();

          kitty::create_nth_var( var, sup.front() );
          if( remainder == var )
          {
            return children[sup.front()];
          }
          else
          {
            assert( remainder == ~var );
            return _ntk.create_not( children[sup.front()] );
          }
          assert( false && "ERROR for primary inputs" );
        }

        /*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
         *   (1) STP 顶层分解（你的算法）
         *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
        {
            auto sig = stp_dsd( _ntk, remainder, children );
            if ( sig != mockturtle::xag_network::signal{} )
            {
                std::cout << "STP 顶层分解成功！" << std::endl;
                return sig; // STP 成功，直接返回
            }
        }



        /*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
         *   (2) bottom decomposition
         *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
        for (auto j = 1u; j < sup.size(); ++j)
        for (auto i = 0u; i < j; ++i)
        {
            auto res = kitty::is_bottom_decomposable(
                remainder, sup[i], sup[j], &remainder, true );
            if (res == kitty::bottom_decomposition::none)
                continue;

            auto copy = children;

            switch (res)
            {
            case kitty::bottom_decomposition::none:
                break;
            case kitty::bottom_decomposition::and_:
                copy[sup[i]] = _ntk.create_and(copy[sup[i]], copy[sup[j]]); break;
            case kitty::bottom_decomposition::or_:
                copy[sup[i]] = _ntk.create_or(copy[sup[i]], copy[sup[j]]); break;
            case kitty::bottom_decomposition::lt_:
                copy[sup[i]] = _ntk.create_lt(copy[sup[i]], copy[sup[j]]); break;
            case kitty::bottom_decomposition::le_:
                copy[sup[i]] = _ntk.create_le(copy[sup[i]], copy[sup[j]]); break;
            case kitty::bottom_decomposition::xor_:
                copy[sup[i]] = _ntk.create_xor(copy[sup[i]], copy[sup[j]]); break;
            }

            return decompose(remainder, copy);
        }

        /*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
         *   (3) Shannon
         *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
        if (sup.size() > 4 || !_ps.with_npn4)
        {
            auto v = sup.front();
            auto c0 = kitty::cofactor0(remainder, v);
            auto c1 = kitty::cofactor1(remainder, v);

            auto f0 = decompose(c0, children);
            auto f1 = decompose(c1, children);

            return _ntk.create_ite(children[v], f1, f0);
        }

        /*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
         *   (4) NPN
         *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
        return xag_from_npn(remainder, children);
    }

    /*────────────────────────────────────*
     * NPN 分解（直接复制原版即可）
     *────────────────────────────────────*/
    mockturtle::xag_network::signal xag_from_npn(
        kitty::dynamic_truth_table const& remainder,
        std::vector<mockturtle::xag_network::signal> const& children )
    {
        auto sup = get_func_supports( remainder );
        std::vector<mockturtle::xag_network::signal> small_pis;
        for (auto i : sup)
            small_pis.push_back(children[i]);

        auto copy = remainder;
        auto support = kitty::min_base_inplace(copy);
        auto small_func = kitty::shrink_to(copy, support.size());

        auto config = kitty::exact_npn_canonization(small_func);

        auto func_str = "0x" + kitty::to_hex(std::get<0>(config));
        auto it = opt_xags.find(func_str);
        assert(it != opt_xags.end());

        std::vector<mockturtle::xag_network::signal> pis(support.size());
        std::copy(small_pis.begin(), small_pis.end(), pis.begin());

        auto perm = std::get<2>(config);
        std::vector<mockturtle::xag_network::signal> pis_perm(support.size());

        for (int i = 0; i < support.size(); i++)
            pis_perm[i] = pis[perm[i]];

        auto phase = std::get<1>(config);
        for (int i = 0; i < support.size(); i++)
            if ((phase >> perm[i]) & 1)
                pis_perm[i] = !pis_perm[i];

         auto res = create_xag_from_str( it->second, pis_perm );


        return ((phase >> support.size()) & 1) ? !res : res;
    }

          xag_network::signal create_xag_from_str( const std::string& str, const std::vector<xag_network::signal>& pis_perm )
      {
        std::stack<int> polar;
        std::stack<xag_network::signal> inputs;

        for ( auto i = 0ul; i < str.size(); i++ )
        {
          // operators polarity
          if ( str[i] == '[' || str[i] == '(' || str[i] == '{' )
          {
            polar.push( (i > 0 && str[i - 1] == '!') ? 1 : 0 );
          }

          //input signals
          if ( str[i] >= 'a' && str[i] <= 'd' )
          {
            inputs.push( pis_perm[str[i] - 'a'] );

            polar.push( ( i > 0 && str[i - 1] == '!' ) ? 1 : 0 );
          }

          //create signals
          if ( str[i] == ']' )
          {
            assert( inputs.size() >= 2u );
            auto x1 = inputs.top();
            inputs.pop();
            auto x2 = inputs.top();
            inputs.pop();

            assert( polar.size() >= 3u );
            auto p1 = polar.top();
            polar.pop();
            auto p2 = polar.top();
            polar.pop();

            auto p3 = polar.top();
            polar.pop();

            inputs.push( _ntk.create_xor( x1 ^ p1, x2 ^ p2 ) ^ p3 );
            polar.push( 0 );
          }

          if ( str[i] == ')' )
          {
            assert( inputs.size() >= 2u );
            auto x1 = inputs.top();
            inputs.pop();
            auto x2 = inputs.top();
            inputs.pop();

            assert( polar.size() >= 3u );
            auto p1 = polar.top();
            polar.pop();
            auto p2 = polar.top();
            polar.pop();

            auto p3 = polar.top();
            polar.pop();

            inputs.push( _ntk.create_and( x1 ^ p1, x2 ^ p2 ) ^ p3 );
            polar.push( 0 );
          }

          if ( str[i] == '}' )
          {
            assert( inputs.size() >= 2u );
            auto x1 = inputs.top();
            inputs.pop();
            auto x2 = inputs.top();
            inputs.pop();

            assert( polar.size() >= 3u );
            auto p1 = polar.top();
            polar.pop();
            auto p2 = polar.top();
            polar.pop();

            auto p3 = polar.top();
            polar.pop();

            inputs.push( _ntk.create_or( x1 ^ p1, x2 ^ p2 ) ^ p3 );
            polar.push( 0 );
          }
        }

        assert( !polar.empty() );
        auto po = polar.top();
        polar.pop();
        return inputs.top() ^ po;
      }
      
          /**
           * @brief 执行STP分解算法的主入口函数
           * 
           * 该函数是STP（Sum-of-Products）分解算法的顶层调用接口，
           * 通过调用内部decompose方法实现逻辑函数的分解。
           * 
           * @return xag_network::signal 分解后生成的XAG网络信号
           */
          xag_network::signal run()
      {
        return decompose( _func, pis );
      }


private:
    mockturtle::xag_network& _ntk;
    kitty::dynamic_truth_table _func;
    std::vector<mockturtle::xag_network::signal> pis;
    std::unordered_map<std::string, std::string>& opt_xags;
    xag_dec_params const& _ps;
};

/*─────────────────────────────────────────────────────────*
 *  外部接口（给 lut_resyn 调用）
 *─────────────────────────────────────────────────────────*/
inline mockturtle::xag_network::signal stp_dec(
    mockturtle::xag_network& ntk,
    kitty::dynamic_truth_table const& func,
    std::vector<mockturtle::xag_network::signal> const& children,
    std::unordered_map<std::string, std::string>& opt_xags,
    xag_dec_params const& ps = {} )
{
    stp_dec_impl impl(ntk, func, children, opt_xags, ps);
    return impl.run();
}


template<class Ntk>
class xag_stp_dec_resynthesis
{
  public:
    explicit xag_stp_dec_resynthesis( std::unordered_map<std::string, std::string>& opt_xags, xag_dec_params const& ps = {} )
      : _opt_xags( opt_xags ),
        _ps( ps )
    {
    }

    template<typename LeavesIterator, typename Fn>
    void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
    {
      std::vector<typename Ntk::signal> children( begin, end );
      const auto f = stp_dec( ntk, function, children, _opt_xags, _ps );
      fn( f );
    }

  private:
    std::unordered_map<std::string, std::string>& _opt_xags;
    xag_dec_params _ps;
};


} // namespace also

#endif
