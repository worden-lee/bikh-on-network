#include "networks.h"
#include "CSVController.h"
#include "BoostDotDisplay.h"
#include "CascadeParameters.h"
#include <boost/random/bernoulli_distribution.hpp>

template<typename network_t, typename params_t>
class update_rule
{
};

template<typename network_t, typename params_t, typename rng_t>
class bikh_on_network_update_rule : public update_rule<network_t, params_t>
{
public:
  typedef class node_state {
	public:
		bool decided;
		bool adopted;
		bool cascaded;
		bool flipped;
		bool signal;
		int neighbors_decided;
		int neighbors_adopted;
		double time_of_decision;
		node_state() : decided(false), adopted(false), cascaded(false),
			flipped(false), signal(false), time_of_decision(0) {}
	}  node_state_t;
	typedef class state_container : public vector<node_state_t> {
	public:
		state_container(int n) : vector<node_state_t>(n) {}
		unsigned n_adopting(void) {
			unsigned n = 0;
			for (typename vector<node_state_t>::iterator it = this->begin(); 
						it != this->end(); ++it)
				if (it->decided && it->adopted)
					++n;
			return n;
		}
		unsigned n_cascading(void) {
			unsigned n = 0;
			for (typename vector<node_state_t>::iterator it = this->begin(); 
						it != this->end(); ++it)
				if (it->decided && it->cascaded)
					++n;
			return n;
		}
		unsigned n_decided(void) {
			unsigned n = 0;
			for (typename vector<node_state_t>::iterator it = this->begin(); 
						it != this->end(); ++it)
				if (it->decided)
					++n;
			return n;
		}
	} state_container_t;
	typedef typename boost::graph_traits<network_t>::vertex_descriptor vertex_index_t;
	
	double p;
	rng_t &rng;

	bikh_on_network_update_rule(double _p, rng_t &_rng) : 
		p(_p), rng(_rng)
	{}

	void update(vertex_index_t i, double t, 
			network_t &n, state_container_t &state)
	{ boost::bernoulli_distribution<> d(p);
		boost::variate_generator<rng_t&, boost::bernoulli_distribution<> > choose_signal(rng, d);
	 	state[i].signal = choose_signal();
		this->figure_out_update(i, state[i].signal, n, state);
		state[i].decided = true;
		state[i].time_of_decision = t;
	}

	virtual void figure_out_update(vertex_index_t i, bool signal,
			network_t &n, state_container_t &state)
	{ state[i].adopted = false;
	}
};

template<typename network_t, typename params_t, typename rng_t>
class pluralistic_ignorance_update_rule : 
	public bikh_on_network_update_rule<network_t,params_t,rng_t>
{
public:
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::state_container_t 
		state_container_t;
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::vertex_index_t 
		vertex_index_t;

	pluralistic_ignorance_update_rule(double p, rng_t &__rng) :
		bikh_on_network_update_rule<network_t,params_t,rng_t>(p,__rng) {}

	virtual void figure_out_update(vertex_index_t i, bool signal,
		 network_t &n, state_container_t &state)
	{ typename graph_traits<network_t>::out_edge_iterator ei,eend;
		unsigned n_decided = 0, n_adopted = 0;
		for (tie(ei,eend) = out_edges(i,n); ei != eend; ++ei)
		{ vertex_index_t j = target(*ei,n);
			if (state[j].decided)
			{ ++n_decided;
				if (state[j].adopted)
					++n_adopted;
			}
		}
		int n_up = n_adopted, n_down = n_decided - n_adopted;
		if (signal) ++n_up; else ++n_down;
		if (n_up > n_down)
		{ state[i].adopted = true;
			if (n_adopted > n_decided - n_adopted + 1) // would adopt with down signal
				state[i].cascaded = true;
		}
		else if (n_up < n_down)
		{ state[i].adopted = false;
			if (n_adopted + 1 < n_decided - n_adopted) // would adopt with up signal
				state[i].cascaded = true;
		}
		else
		{ state[i].adopted = bernoulli_distribution<>(0.5)(this->rng);
			//cout << "coin flip: " << state[i].adopted << "\n";
			state[i].flipped = true;
		}
		state[i].neighbors_adopted = n_adopted;
		state[i].neighbors_decided = n_decided;
		//cout << n_up << " of " << total_n << ": "
		//	<< (state[i].adopted ? "adopted" : "rejected")
		//	<< (state[i].cascaded? ", cascaded" : "")
		//	<< (state[i].flipped?  ", flipped": "") << "\n";
	}
};

template<typename network_t, typename params_t>
class network_dynamics
{
public:
	network_dynamics(network_t &n0, params_t &p0) {}
	void step(void) {}
};

template<typename network_t, typename params_t, 
	typename update_rule_t, typename rng_t>
class update_everyone_once_dynamics : 
	public network_dynamics<network_t,params_t>
{
public:
	typedef typename update_rule_t::vertex_index_t vertex_index_t;
	typedef typename update_rule_t::state_container_t state_container_t;
	state_container_t _state;
	network_t &n;
	params_t &params;
	double _t;

	deque<vertex_index_t> yet_to_update, already_updated;
	update_rule_t updater;

	update_everyone_once_dynamics(network_t &_n, params_t &_params, rng_t &rng_initializer) :
			n(_n), params(_params), _state(num_vertices(_n)), 
			updater(_params.p(),rng_initializer), _t(0),
			network_dynamics<network_t,params_t>(_n,_params)	
	{ typename graph_traits<network_t>::vertex_iterator si,send;
		tie(si,send) = vertices(n);
		copy(si, send, inserter(yet_to_update, yet_to_update.begin()));
		random_shuffle(yet_to_update.begin(), yet_to_update.end());
	}
	
	double t() { return _t; }
	state_container_t &state() { return _state; }

	void step()
	{ if (int(_t) % 100 == 0)
			cout << _t << "\n";
		vertex_index_t i = yet_to_update.front();
		yet_to_update.pop_front();
		already_updated.push_back(i);
		updater.update(i, _t, n, _state);
		_t += 1;
	}
};

template<typename network_t, typename state_t>
class color_cascade_state : public color_vertices<network_t>
{
public:
  typedef typename color_vertices<network_t>::key_type key_type;
  typedef typename color_vertices<network_t>::value_type value_type;
  using color_vertices<network_t>::n;
	state_t &state;

  color_cascade_state(network_t&_n, state_t&_s) : 
		color_vertices<network_t>(_n), state(_s) {}
  value_type operator[](const key_type &e) const
  { return (*this)(e,n); }

  value_type operator()(const key_type &e, const network_t&c) const
	{ if (!state[e].decided)
			return "white";
		else if (!state[e].adopted && !state[e].cascaded)
			return "red";
		else if (!state[e].adopted && state[e].cascaded)
			return "pink";
		else if (state[e].adopted && !state[e].cascaded)
			return "blue";
		else if (state[e].adopted && state[e].cascaded)
			return "lightblue";
	}
};

template<typename network_t, typename state_t>
color_cascade_state<network_t, state_t> 
	make_color_cascade_state(network_t&n, state_t &s)
{ return color_cascade_state<network_t,state_t>(n,s);
}

template<typename network_t, typename params_t, typename rng_t>
void do_cascade(network_t &n, params_t &parameters, rng_t &rng_arg)
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

	typedef pluralistic_ignorance_update_rule<network_t, params_t, rng_t> 
		update_rule_t;
	typedef update_everyone_once_dynamics<network_t, params_t, 
		update_rule_t, rng_t> dynamics_t;
	dynamics_t cascade_dynamics(n, parameters, rng_arg);

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
		<< "decided" << "adopted" << "cascaded" << "flipped" << "signal" 
		<< "neighbors decided" << "neighbors adopted" << endl;

	while (1)
	{ timeseries_csv << cascade_dynamics.t() 
			<< cascade_dynamics.state().n_adopting()
		  << (cascade_dynamics.state().n_adopting() / 
				   (double)cascade_dynamics.state().n_decided());
		timeseries_csv.newRow();
		if (parameters.initial_graph_type() == "LATTICE")
		{ // fix the hell out of this
			int dim0 = string_to_unsigned(*parameters.get("lattice_dim_0"));
			if ( ! cascade_dynamics.already_updated.empty() )
			{ typename update_rule_t::vertex_index_t i = 
					cascade_dynamics.already_updated.back();
				int x = i % dim0, y = i / dim0;
				microstate_csv << cascade_dynamics.t() << x << y << i
					<< cascade_dynamics.state()[i].decided
					<< cascade_dynamics.state()[i].adopted
					<< cascade_dynamics.state()[i].cascaded
					<< cascade_dynamics.state()[i].flipped
					<< cascade_dynamics.state()[i].signal 
					<< cascade_dynamics.state()[i].neighbors_decided
					<< cascade_dynamics.state()[i].neighbors_adopted 
					<< endl;
			}
		}
		if (cascade_dynamics.state().n_decided() >= num_vertices(n))
			break;
		cascade_dynamics.step();
	}

	CSVDisplay summary_csv(parameters.outputDirectory()+"/summary.csv");
	summary_csv << "p" << "proportion adopting" << "proportion cascading" << endl;
	summary_csv << parameters.p()
		<< (cascade_dynamics.state().n_adopting() /
				 (double)cascade_dynamics.state().n_decided())
		<< (cascade_dynamics.state().n_cascading() /
				 (double)cascade_dynamics.state().n_decided())
		<< endl;
}
