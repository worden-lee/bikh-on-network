#include "cascade.h"
#include "networks.h"
#include "CSVController.h"
#include "CascadeParameters.h"

template<typename network_t, typename params_t>
void do_cascade(network_t n, params_t parameters)
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

