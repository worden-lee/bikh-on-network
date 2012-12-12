#include "networks.h"
#include "CSVController.h"
#include "BoostDotDisplay.h"
#include "CascadeParameters.h"
#include <boost/random/bernoulli_distribution.hpp>

// classes for doing the Bikhchandani and related update rules

// abstract base class
template<typename network_t, typename params_t>
class update_rule
{
};

// parent class for several variants on the Bikh. process with
// local interactions
template<typename network_t, typename params_t, typename rng_t>
class bikh_on_network_update_rule : public update_rule<network_t, params_t>
{
public:
  typedef class node_state {
	public:
    // these two are the actual state
		bool decided;
		bool adopted;
    // the rest is extra record-keeping
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

  // this is the actual update operation, which is implemented by subclasses
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

// Pluralistic ignorance is the simple majority approximation: 
// count all the others' actions together with your signal, and go with
// either adoption or rejection according to which is the majority.
// This is a pluralistic ignorance rule because this is what you would
// get if you incorrectly assume all other players' actions match their
// private signals.
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
			if (n_adopted + 1 < n_decided - n_adopted) // would reject with up signal
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

#if 0   // to log messages
#define LOG_OUT cout
#else   // or not to log messages
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/null.hpp"
boost::iostreams::stream< boost::iostreams::null_sink > nullOstream( ( boost::iostreams::null_sink() ) );
#define LOG_OUT nullOstream
#endif

template<typename network_t, typename params_t, typename rng_t>
class bikh_log_odds_update_rule : 
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

  float rho, sigma;
	bikh_log_odds_update_rule(double p, rng_t &__rng) :
		bikh_on_network_update_rule<network_t,params_t,rng_t>(p,__rng),
    rho( log(p/(1-p)) ), sigma( log((1+p)/(2-p)) ) {}

	float sum_of_influences(vector<vertex_index_t>&neighbors, 
			network_t &n, state_container_t &state, string indent = "")
	{ 
		// in order, try to infer their signal from what they were looking at
		float total_log_odds = 0;
		//LOG_OUT << indent << "In sum_of_influences with";
		//for ( unsigned n1 = 0; n1 < neighbors.size(); ++n1 )
		//	LOG_OUT << ' ' << neighbors[n1];
		//LOG_OUT <<"\n";
		vector<vertex_index_t> n1_neighbors;
		for ( unsigned n1 = 0; n1 < neighbors.size(); ++n1 )
		{ n1_neighbors.clear();
			LOG_OUT << indent << "  neighbor " << neighbors[n1] << " (action "
				<< state[neighbors[n1]].adopted << ") sees";
			// what were they looking at?
			for ( unsigned n2 = 0; n2 < n1; ++n2 )
			{ typename boost::graph_traits<network_t>::edge_descriptor e;
				bool edge_exists;
				tie(e, edge_exists) = edge(neighbors[n1],neighbors[n2],n);
				if (edge_exists)
				{ LOG_OUT << ' ' << neighbors[n2];
					n1_neighbors.push_back(neighbors[n2]);
				}
			}
			LOG_OUT << "\n";
			float n1_sum_log_odds = 
				sum_of_influences(n1_neighbors, n, state, indent + "  ");
			LOG_OUT << indent << "    sum of log_odds: " << n1_sum_log_odds << "\n";

			// given the sum_influence and their actual action, there are cases.
			float log_alpha_n1;
			int action = (state[neighbors[n1]].adopted ? 1 : -1);
			// if the influence is enough to override their signal, we don't
			// know their signal because they were part of a cascade
			if (n1_sum_log_odds * action > rho)
				log_alpha_n1 = 0;
			// if the influence is ± rho and they agreed with it, they might have
			// flipped a coin
			else if (n1_sum_log_odds * action == rho)
				log_alpha_n1 = action * sigma;
			// otherwise the influence is too weak and we're seeing their signal.
      // this includes the "paradoxical" case.
			else
				log_alpha_n1 = action * rho;
			LOG_OUT << indent << "    log alpha: " << log_alpha_n1 << "\n";
			total_log_odds += log_alpha_n1;
		}
		return total_log_odds;
	}

	virtual void figure_out_update(vertex_index_t i, bool signal,
		 network_t &n, state_container_t &state)
	{ 
		// reconstruct everyone's decisions and try to infer their signals
		LOG_OUT << "Update site " << i << " (signal " << signal << "):\n";

		// get everyone in the neighborhood, from first to play, to last
		typename graph_traits<network_t>::adjacency_iterator ai,aend;
		vector<vertex_index_t> neighbors;
		unsigned n_decided = 0, n_adopted = 0;
		for (tie(ai,aend) = adjacent_vertices(i, n); ai != aend; ++ai)
		{ vertex_index_t j = *ai;
			if (state[j].decided)
			{ ++n_decided;
				if (state[j].adopted)
					++n_adopted;
				neighbors.push_back(j);
			}
		}
		unsigned n_up = 0, n_down = 0;
		sort( neighbors.begin(), neighbors.end(), compare_times(state) );

		// what are all the neighbors' signals worth to me?
		float total_log_alphas = sum_of_influences(neighbors, n, state);

		// given the total influence and the signal, decide what to do
		float rho_i = (signal ? rho : -rho);
		LOG_OUT << "  rho_i = " << rho_i << "\n";
		LOG_OUT << "  sum of log odds = " << (total_log_alphas + rho_i) << ": ";
		if (total_log_alphas + rho_i > 0)
		{ state[i].adopted = true;
			LOG_OUT << 1;
			if (total_log_alphas > rho)
			{ state[i].cascaded = true;
				LOG_OUT << " (cascade)";
			}
		}
		else if (total_log_alphas + rho_i < 0)
		{ state[i].adopted = false;
			LOG_OUT << -1;
			if (total_log_alphas < -rho)
			{ state[i].cascaded = true;
				LOG_OUT << " (cascade)";
			}
		}
		else
		{ state[i].adopted = bernoulli_distribution<>(0.5)(this->rng);
			LOG_OUT << "coin flip: " << (state[i].adopted ? 1 : -1);
			state[i].flipped = true;
		}
	  LOG_OUT << "\n";
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

