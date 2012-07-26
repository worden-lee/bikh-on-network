//  -*- C++ -*-
//=======================================================================
// libboost-based simulation of Bikhchandani information cascade
// model with local connections and heuristic decision rule
//=======================================================================
#include <boost/config.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/graph/grid_graph.hpp>
//#include <boost/graph/adjacency_list.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/lambda/lambda.hpp>
#include <math.h>
#include "networks.h"
#include "CSVController.h"

#include "CascadeParameters.h"
typedef CascadeParameters ParamsClass;

using namespace boost;
using namespace boost::lambda;
using namespace std;

ParamsClass parameters;

template<typename network_t>
void do_cascade(network_t n)
{
  // ===== do the work =====

  //network_selection_indicator<network_t,rng_t,ParamsClass> sind(rng);
  //sind.inheritParametersFrom(parameters);
  //mean_fixation_time_indicator<typeof sind> ft_ind(sind);
  //mean_extinction_time_indicator<typeof sind> et_ind(sind);
  //mean_absorption_time_indicator<typeof sind> at_ind(sind);
  //mean_peak_before_extinction_indicator<typeof sind> mp_ind(sind);

  double fixp = 0;
  //network_selection_indicator<network_t,rng_t,ParamsClass>::fixation_stats_t
    //stats;


  CSVDisplay nodes_csv(parameters.outputDirectory()+"/nodes.csv");
  nodes_csv << "vertex" << "in degree" << "out degree";
  if (fixp > 0) nodes_csv << "p";
  nodes_csv.newRow();
  typename graph_traits<network_t>::vertex_iterator it,iend;
  for (tie(it,iend) = vertices(n); it != iend; ++it)
  { //fixation_record &vstats = stats.fixations_by_vertex[*it];
    nodes_csv << *it
              << in_degree(*it,n) << out_degree(*it,n);
    //if (fixp > 0)
     // nodes_csv << (double(vstats.n_fixations)/
      //              (vstats.n_fixations+vstats.n_extinctions));
    nodes_csv.newRow();
  }
}

rng_t rng;

// ------------------------------------------------------------------------
//  main
// ------------------------------------------------------------------------

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
	typedef adjacency_list<setS,vecS,bidirectionalS> network_t;
  network_t n(parameters.n_vertices());
	// network created empty, now initialize it  
	//n.inheritParametersFrom(parameters);
	construct_network(n,parameters,rng);

	if (parameters.print_stuff())
	{ if (parameters.print_adjacency_matrix())
		{ cout << "network:\n";
			//print_object(n,cout);
		}
		//cout << "density: " << density(n) << endl;
		//cout << "moran probability: "
			//	 << moran_probability(parameters.mutantFitness()
			//												/parameters.residentFitness(),
			//												parameters.n_vertices()) << '\n';
	}
	do_cascade(n);
}
