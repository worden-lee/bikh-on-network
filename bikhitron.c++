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
#include "networks.h"
#include "cascade.h"

#include "CascadeParameters.h"
typedef CascadeParameters ParamsClass;

using namespace boost;
using namespace boost::lambda;
using namespace std;

// ------------------------------------------------------------------------
//  main
// ------------------------------------------------------------------------

rng_t global_rng;

int
main(int argc, char **argv)
{  
  // ===== initialize =====
	ParamsClass parameters;
  parameters.handleArgs(argc,argv,"settings/defaults-cascade.settings");
  parameters.afterSetting();
	parameters.finishInitialize();
	{ ofstream settings_csv((parameters.outputDirectory()+"/settings.csv").c_str());
		parameters.writeAllSettingsAsCSV(settings_csv);
	}

	cout << "randSeed is " << parameters.randSeed() << endl;
  global_rng.seed(parameters.randSeed());
  // rand() is used in random_shuffle()
  srand(parameters.randSeed());
  
  // ===== create a random or custom graph =====
	typedef adjacency_list<setS,vecS,bidirectionalS> network_t;
  network_t n(parameters.n_vertices());
	// network created empty, now initialize it  
	//n.inheritParametersFrom(parameters);
	construct_network(n,parameters,global_rng);

	if (parameters.print_stuff())
	{ if (parameters.print_adjacency_matrix())
		{ cout << "network:\n";
			print_object(n,cout);
		}
		//cout << "density: " << density(n) << endl;
	}
	do_cascade(n, parameters, global_rng);
}
