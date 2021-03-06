//  -*- C++ -*-
//=======================================================================
// libboost-based simulation of Bikhchandani information cascade
// model with local connections and heuristic decision rule
//=======================================================================
#include <boost/config.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/lambda/lambda.hpp>
#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include "networks.h"
#include "cascade.h"

#include "CascadeParameters.h"

using namespace boost;
using namespace boost::lambda;
using namespace std;

// ------------------------------------------------------------------------
//  do_cascade()
// ------------------------------------------------------------------------

template<typename network_t, typename cascade_dynamics_t, typename params_t>
void do_cascade(network_t &n, cascade_dynamics_t &cascade_dynamics,
		params_t &parameters) {
	// ===== do the work =====

	/*
	CSVDisplay nodes_csv(parameters.outputDirectory()+"/degrees.csv");
	nodes_csv << "vertex" << "in degree" << "out degree";
	nodes_csv.newRow();
	typename graph_traits<network_t>::vertex_iterator it,iend;
	for (tie(it,iend) = vertices(n); it != iend; ++it) { 
		nodes_csv << *it << in_degree(*it,n) << out_degree(*it,n);
		nodes_csv.newRow();
	}

	if (parameters.initial_graph_type() != "LATTICE") {
	       	CSVDisplay network_csv(parameters.outputDirectory()+"/network.csv");
		network_csv << "vertex" << "neighbors" << endl;
		typename graph_traits<network_t>::vertex_iterator vi,vend;
		for (tie(vi,vend) = vertices(n); vi != vend; ++vi) {
		       	network_csv << *vi;
			ostringstream nbrs;
			typename graph_traits<network_t>::adjacency_iterator ai,aend;
			for (tie(ai,aend) = adjacent_vertices(*vi, n); ai != aend; ++ai) {
				nbrs << *ai << " ";
			}
			network_csv << nbrs.str() << endl;
		}
	}
	*/
	struct timeval before_time, after_time;
	if ( gettimeofday( &before_time, NULL ) )
		perror( "gettimeofday" );

	CSVDisplay timeseries_csv(parameters.outputDirectory()+"/measurements.csv");
	timeseries_csv << "t" << "n_adopting" << "proportion_adopting";
	timeseries_csv.newRow();

	if (0) {
	BoostDotGraphController<network_t, params_t> graph_animation;
	graph_animation.add_vertex_property("color",
			make_color_cascade_state(n, cascade_dynamics.state()));
	graph_animation.params.setdisplayToScreen(true);
	graph_animation.inheritParametersFrom(parameters);
	}
	CSVDisplay microstate_csv(parameters.outputDirectory()+"/microstate.csv");
	microstate_csv << "t" << "x" << "y" << "index" 
		<< "decided" << "signal" << "adopted" << "cascaded" << "flipped"
		<< "neighbors decided" << "neighbors adopted" << endl;

	while (1) {
	       	timeseries_csv << cascade_dynamics.t() 
			<< cascade_dynamics.state().n_adopting()
			<< (cascade_dynamics.state().n_adopting() / 
					 (double)cascade_dynamics.state().n_decided());
		timeseries_csv.newRow();
		//if (parameters.initial_graph_type() == "LATTICE") {
		// fix the hell out of this
			int dim0;
			if (parameters.initial_graph_type() == "LATTICE") {
		 		dim0 = string_to_unsigned(*parameters.get("lattice_dim_0"));
			} else { // fake lattice shape for other network
				dim0 = ceil(sqrt((float)num_vertices(n)));
			}
			if ( ! cascade_dynamics.already_updated.empty() ) {
			       	typename cascade_dynamics_t::vertex_index_t i = 
					cascade_dynamics.already_updated.back();
				int x = i % dim0, y = i / dim0;
				microstate_csv << cascade_dynamics.t() << x << y << i
					<< cascade_dynamics.state()[i].decided
					<< cascade_dynamics.state()[i].signal 
					<< cascade_dynamics.state()[i].adopted
					<< cascade_dynamics.state()[i].cascaded
					<< cascade_dynamics.state()[i].flipped
					<< cascade_dynamics.state()[i].neighbors_decided
					<< cascade_dynamics.state()[i].neighbors_adopted 
					<< endl;
			}
		//}
		if (cascade_dynamics.state().n_decided() >= num_vertices(n)) {
			break;
		}
		cascade_dynamics.step();
	}

	if ( gettimeofday( &after_time, NULL ) )
		perror( "gettimeofday" );

	CSVDisplay summary_csv(parameters.outputDirectory()+"/summary.csv");
	summary_csv << "random seed" << "p" << "neighbors" << "update rule" 
		<< "inference closure level"
		<< "population size" << "proportion adopting" << "proportion cascading" 
		<< "last action" << "running time" << "inferences" << "memoizes" << endl;
	extern int n_inferences, n_memoized;
	summary_csv << fstring("%.3ld",parameters.randSeed()) 
		<< parameters.p() << parameters.n_neighbors() 
		<< parameters.update_rule()
		<< parameters.inference_closure_level() << num_vertices(n)
		<< (cascade_dynamics.state().n_adopting() /
			 (double)cascade_dynamics.state().n_decided())
		<< (cascade_dynamics.state().n_cascading() /
			 (double)cascade_dynamics.state().n_decided())
		<< cascade_dynamics.state()[cascade_dynamics.already_updated.back()].adopted
		<< ((after_time.tv_sec + after_time.tv_usec / 1000000.0)
			- (before_time.tv_sec + before_time.tv_usec / 1000000.0))
		<< n_inferences
		<< n_memoized
		<< endl;
}

// ------------------------------------------------------------------------
//	main
// ------------------------------------------------------------------------

int main(int argc, char **argv) {	
	// ===== initialize =====
	CascadeParameters parameters;
	parameters.handleArgs(argc,argv,"settings/defaults-cascade.settings");
	parameters.afterSetting();
	parameters.finishInitialize(); {
	       	ofstream settings_csv((parameters.outputDirectory()+"/settings.csv").c_str());
		parameters.writeAllSettingsAsCSV(settings_csv);
	}

	rng_t main_rng;

	cout << "randSeed is " << parameters.randSeed() << endl;
	main_rng.seed(parameters.randSeed());
	// rand() is used in random_shuffle()
	srand(parameters.randSeed());
	
	// ===== create a random or custom graph =====
	typedef adjacency_list<setS,vecS,bidirectionalS> network_t;
	network_t n(parameters.n_vertices());
	// network created empty, now initialize it	
	//n.inheritParametersFrom(parameters);
	construct_network(n,parameters,main_rng);

	if (parameters.print_stuff()) {
	       	if (parameters.print_adjacency_matrix()) {
		       	cout << "network:\n";
			print_object(n,cout);
		}
		//cout << "density: " << density(n) << endl;
	}

	if (parameters.update_rule() == "counting") {
	       	typedef counting_update_rule<network_t, CascadeParameters, rng_t>
			update_rule_t;
		typedef update_everyone_once_dynamics<network_t, CascadeParameters, 
			update_rule_t, rng_t> dynamics_t;
		dynamics_t dynamics(n, parameters, main_rng);
		do_cascade(n, dynamics, parameters);
	} else if (parameters.update_rule() == "bayesian") {
	       	typedef bikh_log_odds_update_rule<network_t, CascadeParameters, rng_t> 
			update_rule_t;
		typedef update_everyone_once_dynamics<network_t, CascadeParameters, 
			update_rule_t, rng_t> dynamics_t;
		dynamics_t dynamics(n, parameters, main_rng);
		do_cascade(n, dynamics, parameters);
	} else if (parameters.update_rule() == "bayesian-same-neighborhood") {
	       	typedef same_neighborhood_log_odds_update_rule<network_t, CascadeParameters, rng_t> 
			update_rule_t;
		typedef update_everyone_once_dynamics<network_t, CascadeParameters, 
			update_rule_t, rng_t> dynamics_t;
		dynamics_t dynamics(n, parameters, main_rng);
		do_cascade(n, dynamics, parameters);
	} else if (parameters.update_rule() == "bayesian-closure") {
	       	typedef counting_closure_log_odds_update_rule<network_t, CascadeParameters, rng_t> 
			update_rule_t;
		typedef update_everyone_once_dynamics<network_t, CascadeParameters, 
			update_rule_t, rng_t> dynamics_t;
		dynamics_t dynamics(n, parameters, main_rng);
		do_cascade(n, dynamics, parameters);
	} else {
	       	cerr << "error: what is the update rule??" << endl;
	}
}
