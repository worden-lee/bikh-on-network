// -*- C++ -*-
#ifndef __network_experiment_parameters_h__
#define __network_experiment_parameters_h__
#include "Parameters.h"
#include <boost/random.hpp>

class NetworkExperimentParameters : public Parameters
{
public:
  // ===== declarations of parameters =====

  // --- all are declared in Parameters

};

// -----------------------------------------

// global random number generator, used throughout
typedef boost::minstd_rand rng_t;
extern rng_t rng;

// use these types to actually get random numbers
typedef boost::variate_generator<rng_t&, boost::uniform_int<> >  ui_t;
typedef boost::variate_generator<rng_t&, boost::uniform_real<> > ur_t;

#endif // __network_experiment_parameters_h__
