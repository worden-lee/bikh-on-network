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

	// size of lattice.  lattice_dim0, lattice_dim1, etc can be used,
	// if not found this one is used
	DECLARE_PARAM(unsigned, lattice_dim_generic)

	// extent of neighborhood - first neighbors, second, ...
	DECLARE_PARAM(unsigned, neighbor_depth)

  // whether coordinates wrap around edges
  DECLARE_PARAM(bool, lattice_is_torus)

};

// -----------------------------------------

// global random number generator, used throughout
typedef boost::minstd_rand rng_t;
extern rng_t rng;

// use these types to actually get random numbers
typedef boost::variate_generator<rng_t&, boost::uniform_int<> >  ui_t;
typedef boost::variate_generator<rng_t&, boost::uniform_real<> > ur_t;

#endif // __cascade_parameters_h__
