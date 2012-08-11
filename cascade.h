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

	// used by multiple subclasses for sorting
	class compare_times
	{ public:
		  const state_container_t &state;
			compare_times(const state_container_t&s) : state(s) {}
			bool operator()(const vertex_index_t &a, const vertex_index_t &b)
			{ return state[a].time_of_decision < state[b].time_of_decision;
			}
	};

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
	using bikh_on_network_update_rule<network_t,params_t,rng_t>::compare_times;

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

template<typename network_t, typename params_t, typename rng_t>
class approximate_inference_update_rule : 
	public bikh_on_network_update_rule<network_t,params_t,rng_t>
{
public:
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::state_container_t 
		state_container_t;
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::vertex_index_t 
		vertex_index_t;
	typedef typename
	 	bikh_on_network_update_rule<network_t,params_t,rng_t>::compare_times
		compare_times;

	approximate_inference_update_rule(double p, rng_t &__rng) :
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
		unsigned n_up = 0, n_down = 0;
		// reconstruct everyone's decisions and try to infer their signals

		// get everyone in the neighborhood, from first to play, to last
		typename graph_traits<network_t>::adjacency_iterator ai,aend;
		tie(ai,aend) = adjacent_vertices(i, n);
		vector<vertex_index_t> neighbors( ai, aend );
		sort( neighbors.begin(), neighbors.end(), compare_times(state) );
		vector<float> inferred_signals( neighbors.size(), 0./0. );
		// in order, try to infer their signal from what they were looking at
		float total_influence = 0;
		for ( unsigned n1 = 0; n1 < neighbors.size(); ++n1 )
			if (state[neighbors[n1]].decided)
			{ // what were they looking at?
				float sum_influence = 0;
				for ( unsigned n2 = 0; n2 < n1; ++n2 )
				  if (state[neighbors[n2]].decided)
					{ typename boost::graph_traits<network_t>::edge_descriptor e;
						bool edge_exists;
						tie(e, edge_exists) = edge(neighbors[n1],neighbors[n2],n);
						if (edge_exists)
							sum_influence += inferred_signals[n2];
					}
				// given the sum_influence and their actual action, there are cases.
				int action = (state[neighbors[n1]].adopted ? 1 : -1);
				// if the influence is enough to override their signal, we don't
				// know their signal because they were part of a cascade
				if (sum_influence * action > 1)
					inferred_signals[n1] = 0;
				// if the influence is ±1 and they agreed with it, they might have
				// flipped a coin
				else if (sum_influence * action == 1)
					inferred_signals[n1] = action * 2.0 / 3.0;
				// otherwise the influence is too weak and we're seeing their signal.
				else
					inferred_signals[n1] = action;
				total_influence += inferred_signals[n1];
			}

		// given the total influence and the signal, decide what to do
		int signed_signal = (signal ? 1 : -1);
		//cout << "site " << i << ": ";
		//cout << "total influence = " << total_influence << ", signal = "
		//	<< signed_signal << ": ";
		if (total_influence + signed_signal > 0)
		{ state[i].adopted = true;
			//cout << 1;
			if (total_influence > 1)
			{ state[i].cascaded = true;
				//cout << " (cascade)";
			}
		}
		else if (total_influence + signed_signal < 0)
		{ state[i].adopted = false;
			//cout << -1;
			if (total_influence < -1)
			{ state[i].cascaded = true;
				//cout << " (cascade)";
			}
		}
		else
		{ state[i].adopted = bernoulli_distribution<>(0.5)(this->rng);
			//cout << "coin flip: " << (state[i].adopted ? 1 : -1);
			state[i].flipped = true;
		}
	  //cout << "\n";
		state[i].neighbors_adopted = n_adopted;
		state[i].neighbors_decided = n_decided;
	}
};


template<typename network_t, typename params_t, typename rng_t>
class bayesian_update_rule : 
	public bikh_on_network_update_rule<network_t,params_t,rng_t>
{
public:
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::state_container_t 
		state_container_t;
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::vertex_index_t 
		vertex_index_t;

	bayesian_update_rule(double p, rng_t &__rng) :
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
		unsigned n_up = 0, n_down = 0;
		// reconstruct everyone's decisions and try to infer their signals

		// get everyone in the neighborhood, from first to play, to last
		typename graph_traits<network_t>::adjacency_iterator ai,aend;
		tie(ai,aend) = adjacent_vertices(i, n);
		vector<vertex_index_t> neighbors( ai, aend );
		sort( neighbors.begin(), neighbors.end(), compare_times(state) );
		vector<float> inferred_signals( neighbors.size(), 0./0. );
		// in order, try to infer their signal from what they were looking at
		float total_influence = 0;
		for ( unsigned n1 = 0; n1 < neighbors.size(); ++n1 )
			if (state[neighbors[n1]].decided)
			{ // what were they looking at?
				float sum_influence = 0;
				for ( unsigned n2 = 0; n2 < n1; ++n2 )
				  if (state[neighbors[n2]].decided)
					{ typename boost::graph_traits<network_t>::edge_descriptor e;
						bool edge_exists;
						tie(e, edge_exists) = edge(neighbors[n1],neighbors[n2],n);
						if (edge_exists)
							sum_influence += inferred_signals[n2];
					}
				// given the sum_influence and their actual action, there are cases.
				int action = (state[neighbors[n1]].adopted ? 1 : -1);
				// if the influence is enough to override their signal, we don't
				// know their signal because they were part of a cascade
				if (sum_influence * action > 1)
					inferred_signals[n1] = 0;
				// if the influence is ±1 and they agreed with it, they might have
				// flipped a coin
				else if (sum_influence * action == 1)
					inferred_signals[n1] = action * 2.0 / 3.0;
				// otherwise the influence is too weak and we're seeing their signal.
				else
					inferred_signals[n1] = action;
				total_influence += inferred_signals[n1];
			}

		// given the total influence and the signal, decide what to do
		int signed_signal = (signal ? 1 : -1);
		//cout << "site " << i << ": ";
		//cout << "total influence = " << total_influence << ", signal = "
		//	<< signed_signal << ": ";
		if (total_influence + signed_signal > 0)
		{ state[i].adopted = true;
			//cout << 1;
			if (total_influence > 1)
			{ state[i].cascaded = true;
				//cout << " (cascade)";
			}
		}
		else if (total_influence + signed_signal < 0)
		{ state[i].adopted = false;
			//cout << -1;
			if (total_influence < -1)
			{ state[i].cascaded = true;
				//cout << " (cascade)";
			}
		}
		else
		{ state[i].adopted = bernoulli_distribution<>(0.5)(this->rng);
			//cout << "coin flip: " << (state[i].adopted ? 1 : -1);
			state[i].flipped = true;
		}
	  //cout << "\n";
		state[i].neighbors_adopted = n_adopted;
		state[i].neighbors_decided = n_decided;
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

