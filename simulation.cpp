//  -*- C++ -*-
//=======================================================================
// libboost-based simulation of Bikhchandani information cascade
// model with local connections and heuristic decision rule
//=======================================================================
#include <boost/config.hpp>
#include <boost/tuple/tuple.hpp>
//#include <boost/graph/wavefront.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/lambda/lambda.hpp>
#include <math.h>

#include "CascadeParameters.h"
typedef CascadeParameters ParamsClass;

// undefine DISPLAY to make an executable without any of the display
// classes compiled in
#ifdef DISPLAY
#include "BoostDotDisplay.h"
#include "IndicatorsDisplay.h"
#else
template<typename arg_t>
class DisplayFarm{};
#endif//DISPLAY

using namespace boost;
using namespace boost::lambda;
using namespace std;

rng_t rng;

// ------------------------------------------------------------------------
//  main
// ------------------------------------------------------------------------

ParamsClass parameters;

typedef adjacency_list<setS,vecS,bidirectionalS> network_t;

int
main(int argc, char **argv)
{  
  // ===== initialize =====

  parameters.handleArgs(argc,argv,"settings/defaults-cascade.settings");
  parameters.afterSetting();

  rng.seed(parameters.randSeed());
  // rand() is used in random_shuffle()
  srand(parameters.randSeed());
  
  // ===== create a random or custom graph =====

  network_t n(parameters.n_vertices());
  // network created empty, now initialize it  
  //n.inheritParametersFrom(parameters);
  construct_network(n,parameters,rng);

  if (parameters.print_stuff())
  { if (parameters.print_adjacency_matrix())
    { cout << "network:\n";
      print_object(n,cout);
    }
    cout << "density: " << density(n) << endl;
    cout << "moran probability: "
         << moran_probability(parameters.mutantFitness()
                              /parameters.residentFitness(),
                              parameters.n_vertices()) << '\n';
  }

  // ===== do the work =====

  network_selection_indicator<network_t,rng_t,ParamsClass> sind(rng);
  sind.inheritParametersFrom(parameters);
  mean_fixation_time_indicator<typeof sind> ft_ind(sind);
  mean_extinction_time_indicator<typeof sind> et_ind(sind);
  mean_absorption_time_indicator<typeof sind> at_ind(sind);
  mean_peak_before_extinction_indicator<typeof sind> mp_ind(sind);

  double fixp = 0;
  network_selection_indicator<network_t,rng_t,ParamsClass>::fixation_stats_t
    stats;

  network_component_status status = ::test_fixation_candidate(n);
  if (parameters.require_strongly_connected() and 
      status.flag != STRONGLY_CONNECTED)
  { cout << "graph is not strongly connected.\n";
    //exit(0);
  }
  else
  { if (status.flag == STRONGLY_CONNECTED)
      cout << "graph is strongly connected." << endl;

    if (!parameters.skipFixation()) {
#ifdef DISPLAY
      // plot a picture of the graph
      BoostDotGraphController<network_t,ParamsClass> opt_animation;
      opt_animation.inheritParametersFrom(parameters);
      if (parameters.n_vertices() < 50) // 40
      { //displayfarm.installController(opt_animation);
        opt_animation.add_vertex_property("color",
          make_color_temperature(n));
        //       opt_animation.add_vertex_property("fontcolor",
        //         make_color_influence(n,indicator,parameters));
        opt_animation.add_edge_property("style",
          make_bool_string_property("bold","",belongs_to_a_2_loop<network_t>,n));
        //opt_animation.params.setdisplayToScreen(
        //  displayfarm.params.animate_optimization());
        opt_animation.update(0,n);
      }
#endif
      // now evaluate the simulation
      fixp = sind(n);

      // now get the simulation results, in detail
      stats = sind.provide_stats(n);
      if (parameters.print_stuff())
      { cout << "fixation probability: " << fixp << '\n';
        cout << "n of trials: " << stats.n_trials << '\n';
        if (fixp > 0)
        {
          cout << "mean fixation time: "   << ft_ind(n) << '\n';
          cout << "mean extinction time: " << et_ind(n) << '\n';
          cout << "mean absorption time: " << at_ind(n) << '\n';
          cout << "mean peak before extinction: " << mp_ind(n) << '\n';
          cout << "biggest peak before extinction: "
               << stats.max_peakbeforeextinction << '\n';
        }
      }

      if (fixp > 0)
      { // record the results
        CSVDisplay network_csv(parameters.outputDirectory()+"/network.csv");
        // record other measures like mu_{-1} ?
        network_csv << "nv" << "in_exp" << "out_exp" << "density" << "mutuality"
                    << "p" << "accuracy" << "moran_probability";
        for (int i = -2; i <= 2; ++i)
          for (int j = -2; j <= 2; ++j)
            if (i != 0 || j != 0)
              network_csv << stringf("mu.%d.%d",i,j);
        network_csv.newRow();
        network_csv << parameters.n_vertices() << parameters.pl_in_exp()
                    << parameters.pl_out_exp() << density(n) << mutuality_ind(n)
                    << fixp << stats.fixation_accuracy()
                    << moran_probability(parameters.mutantFitness()
                                         /parameters.residentFitness(),
                                         parameters.n_vertices());
        for (int i = -2; i <= 2; ++i)
          for (int j = -2; j <= 2; ++j)
            if (i != 0 || j != 0)
              network_csv << degree_moment(n,i,j);
        network_csv.newRow();

      }
    }
  }

  CSVDisplay nodes_csv(parameters.outputDirectory()+"/nodes.csv");
  nodes_csv << "vertex" << "in degree" << "out degree";
  if (fixp > 0) nodes_csv << "p";
  nodes_csv.newRow();
  graph_traits<network_t>::vertex_iterator it,iend;
  for (tie(it,iend) = vertices(n); it != iend; ++it)
  { fixation_record &vstats = stats.fixations_by_vertex[*it];
    nodes_csv << *it
              << in_degree(*it,n) << out_degree(*it,n);
    if (fixp > 0)
      nodes_csv << (double(vstats.n_fixations)/
                    (vstats.n_fixations+vstats.n_extinctions));
    nodes_csv.newRow();
  }

  return 0;
}
