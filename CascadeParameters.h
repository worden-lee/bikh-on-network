// -*- C++ -*-
#ifndef __cascade_parameters_h__
#define __cascade_parameters_h__
#include "Parameters.h"
#include <boost/random.hpp>

class CascadeParameters : public Parameters
{
public:
  // ===== declarations of parameters =====

	// what kind of structure - lattice, network, global
	DECLARE_PARAM(string, initial_graph_type)

	// dimension of lattice
	DECLARE_PARAM(unsigned, lattice_dimensions)

	// size of lattice.  lattice_dim_0, lattice_dim_1, etc can be used,
	// if not found this one is used
	DECLARE_PARAM(unsigned, lattice_dim_generic)

	// extent of neighborhood - first neighbors, second, ...
	DECLARE_PARAM(unsigned, neighborhood_radius)

	// what kind of neighborhood: square nbd in "taxicab" metric,
	// diamond in "infinity" metric
	DECLARE_PARAM(string, lattice_metric)

  // whether coordinates wrap around edges
  DECLARE_PARAM(bool, lattice_is_torus)

	// probability that a given individual's private signal is accurate
	DECLARE_PARAM(double, p)

	// what type of process: pluralistic-ignorance, approximate-inference,
	// or Bayesian
	DECLARE_PARAM(string, update_rule)

	unsigned n_neighbors(void)
	{ unsigned nr = neighborhood_radius();
		string metric = lattice_metric();
		if (metric == "infinity")
			return (nr+1)*(nr+1) - 1;
		else if (metric == "taxicab")
			return nr * (nr+1) / 2;
		else
		{ cerr << "unknown lattice_metric\n";
			return -1;
		}
	}

	void finishInitialize()
	{ if (initial_graph_type() == "LATTICE" && n_vertices() == 0)
		{ int nv = 1;
			for (int i = lattice_dimensions() - 1; i >= 0; --i)
			{ const string *ds = get(string("lattice_dim_")+fstring("%u",i));
				if (!ds) ds = get("lattice_dim_generic");
				nv *= string_to_unsigned(*ds);
			}
		  setn_vertices(nv);
		}
		Parameters::finishInitialize();
	}

	void parseSettingsFile(string filename)
	{ string pathname = filename;
		if (pathname[0] != '/')
		{ pathname = dirnameForSettings() + '/' + filename;
			struct stat statbuf;
			if (!stat(pathname.c_str(), &statbuf))
				return Parameters::parseSettingsFile(pathname);
			pathname = dirnameForSettings() + "/settings/" + filename;
			if (!stat(pathname.c_str(), &statbuf))
				return Parameters::parseSettingsFile(pathname);
		}
		return Parameters::parseSettingsFile(filename);
	}
};

// -----------------------------------------

// global random number generator, used throughout
typedef boost::minstd_rand rng_t;
//typedef boost::mt19937 rng_t;

// use these types to actually get random numbers
typedef boost::variate_generator<rng_t&, boost::uniform_int<> >  ui_t;
typedef boost::variate_generator<rng_t&, boost::uniform_real<> > ur_t;

#endif // __cascade_parameters_h__
