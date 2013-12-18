#include "networks.h"
#include "CSVController.h"
#include "BoostDotDisplay.h"
#include "CascadeParameters.h"
#include <boost/random/bernoulli_distribution.hpp>
using namespace std;

// classes for doing the Bikhchandani and related update rules

// abstract base class for an object to update states of nodes on network,
// one at a time
template<typename network_t, typename params_t>
class update_rule
{
};

// base class for several variants on the Bikh. process with
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
			flipped(false), signal(false), 
			neighbors_decided(0), neighbors_adopted(0),
			time_of_decision(0) {}
	}  node_state_t;
	typedef class state_container : public vector<node_state_t> {
	public:
		state_container(int n) : vector<node_state_t>(n) {}
		unsigned n_adopting(void) {
			unsigned n = 0;
			for (typename vector<node_state_t>::iterator 
			     it = this->begin(); it != this->end(); ++it) {
				if (it->decided && it->adopted) {
					++n;
				}
			}
			return n;
		}
		unsigned n_cascading(void) {
			unsigned n = 0;
			for (typename vector<node_state_t>::iterator 
			     it = this->begin(); it != this->end(); ++it) {
				if (it->decided && it->cascaded) {
					++n;
				}
			}
			return n;
		}
		unsigned n_decided(void) {
			unsigned n = 0;
			for (typename vector<node_state_t>::iterator 
			     it = this->begin(); it != this->end(); ++it) {
				if (it->decided) {
					++n;
				}
			}
			return n;
		}
	} state_container_t;
	typedef typename boost::graph_traits<network_t>::vertex_descriptor 
		vertex_index_t;
	
	double p;
	rng_t &rng;

	bikh_on_network_update_rule(double _p, rng_t &_rng) : 
		p(_p), rng(_rng)
	{}

	void update(vertex_index_t i, double t, 
			network_t &n, params_t &params, state_container_t &state) {
	       	boost::bernoulli_distribution<> d(p);
		boost::variate_generator< rng_t&, boost::bernoulli_distribution<> > 
			choose_signal(rng, d);
	 	state[i].signal = choose_signal();
		this->figure_out_update(i, state[i].signal, n, params, state);
		state[i].decided = true;
		state[i].time_of_decision = t;
	}

	// this is the actual update operation, which is implemented by subclasses
	virtual void figure_out_update(vertex_index_t i, bool signal,
			network_t &n, params_t &params, state_container_t &state) {
	       	state[i].adopted = false;
	}

	// used by multiple subclasses for sorting
	class compare_times {
	public:
		const state_container_t &state;
		compare_times(const state_container_t&s) : state(s) {}
		bool operator()(const vertex_index_t &a, const vertex_index_t &b) {
		       	return state[a].time_of_decision < state[b].time_of_decision;
		}
	};
};

// The counting rule is a simple majority approximation: 
// count all the others' actions together with your signal, and go with
// either adoption or rejection according to which is the majority.
// This is a pluralistic ignorance rule, because this is what you would
// get if you incorrectly assume all other players' actions match their
// private signals.
template<typename network_t, typename params_t, typename rng_t>
class counting_update_rule : 
	public bikh_on_network_update_rule<network_t,params_t,rng_t> {
public:
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::state_container_t 
		state_container_t;
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::vertex_index_t 
		vertex_index_t;

	counting_update_rule(double p, rng_t &__rng) :
		bikh_on_network_update_rule<network_t,params_t,rng_t>(p,__rng) 
		{}

	virtual void figure_out_update(vertex_index_t i, bool signal,
		 network_t &n, params_t &params, state_container_t &state) {
	       	typename graph_traits<network_t>::out_edge_iterator ei,eend;
		unsigned n_decided = 0, n_adopted = 0;
		for (tie(ei,eend) = out_edges(i,n); ei != eend; ++ei) {
		       	vertex_index_t j = target(*ei,n);
			if (state[j].decided) {
			       	++n_decided;
				if (state[j].adopted)
					++n_adopted;
			}
		}
		int n_up = n_adopted, n_down = n_decided - n_adopted;
		++(signal ? n_up : n_down);
		if (n_up > n_down) {
		       	state[i].adopted = true;
			if (n_adopted > n_decided - n_adopted + 1) { 
				// would adopt with down signal
				state[i].cascaded = true;
			}
		}
		else if (n_up < n_down) {
		       	state[i].adopted = false;
			if (n_adopted + 1 < n_decided - n_adopted) {
				// would reject with up signal
				state[i].cascaded = true;
			}
		} else {
		       	state[i].adopted = bernoulli_distribution<>(0.5)(this->rng);
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
boost::iostreams::stream< boost::iostreams::null_sink > null_ostream( ( boost::iostreams::null_sink() ) );
#define LOG_OUT null_ostream
#endif

// This is the Bikhchandani et al model, extended to consider
// possibly-local interactions.
// Each player uses Bayesian inference to infer likelihood that the true
// signal is positive, taking into account other players' actions and their
// own private signal.  In a partly-connected network, each player takes into
// account only the other players they are connected to.  When interpreting
// another player's action, they take into account that that person only saw
// the people they're connected to.  But they can't take into account people
// they themselves can't see, so they proceed as if the neighbors' neighborhood
// is limited to the intersection of the neighbor's true neighborhood with
// their own neighborhood.
// This leads to a combinatorial explosion, because the likelihood
// calculation has to be done over and over for many different subsets of each
// player's neighborhood.
template<typename network_t, typename params_t, typename rng_t>
class bikh_log_odds_update_rule : 
	public bikh_on_network_update_rule<network_t,params_t,rng_t> {
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
	map<vertex_index_t,float> *memoize_sum_of_influences_p;

      	bikh_log_odds_update_rule(double p, rng_t &__rng) :
		bikh_on_network_update_rule<network_t,params_t,rng_t>(p,__rng),
		rho( log(p/(1-p)) ), sigma( log((1+p)/(2-p)) ),
	       	memoize_sum_of_influences_p(NULL) {}

	// Given someone on the network, we figure out what they'll do
	// by looking at each of their neighbors, and reconstructing what
	// each of them did in relation to what their neighbors did...
	// So we need to figure out who a neighbor's neighbors are.  And not
	// just who their neighbors actually are, that would be too easy, but
	// who this person thinks that person's neighbors are, which is the
	// intersection of the two people's neighborhoods.
	// The caller expects this to return only:
	//   nodes connected to n1
	//   which took action before n1 did
	// n1: the person whose neighborhood we want
	// all_neighbors: restricted to this universe
	// n: the network
	// n1_neighbors: container where answer will be returned
	virtual void construct_neighbors_neighbors( vertex_index_t n1,
			set<vertex_index_t> &all_neighbors,
			network_t &n,
			set<vertex_index_t> &n1_neighbors,
			state_container_t &state,
		       	string indent) {
		LOG_OUT << indent << "  construct neighbors of " << n1 << ':';
		for ( typename set<vertex_index_t>::iterator n2i = all_neighbors.begin(); 
		      n2i != all_neighbors.end(); ++n2i ) {
			vertex_index_t n2 = *n2i;
			typename boost::graph_traits<network_t>::edge_descriptor e;
			bool edge_exists;
			tie(e, edge_exists) = edge(n1,n2,n);
			if (edge_exists && state[n2].decided && 
			    state[n2].time_of_decision < state[n1].time_of_decision) {
				n1_neighbors.insert(n2);
				LOG_OUT << ' ' << n2;
			} else {
				LOG_OUT << " (" << n2 << ')';
			}
		}
		LOG_OUT << endl;
	}

	// sum_of_influences is called recursively.  Given the neighbors
	// of a given node, we reconstruct what influenced those neighbors
	// and what we can infer about each of their signals.  Return the
	// sum of log odds expressing the likelihood that the true signal
	// is positive, given that information.
	virtual float sum_of_influences(set<vertex_index_t>&neighbors, 
			vertex_index_t n0,
			network_t &n, params_t &params, 
			state_container_t &state, string indent = "") { 
		// in order, try to infer their signal from what they were looking at
		float total_log_odds = 0;

                // if we're in a complete graph, we can cache results and not recompute them,
                // just based on the number who have played so far
		if (memoize_sum_of_influences_p && memoize_sum_of_influences_p->count(n0) > 0) {
			float lookup = (*memoize_sum_of_influences_p)[n0];
			LOG_OUT << indent << "  sum_of_influences(" << n0 << ") memoized: " << lookup << endl;
			return lookup;
		}

		// Now compute the log odds likelihood thing for each of
		// the neighbors we're given
		set<vertex_index_t> n1_neighbors;
		for ( typename set<vertex_index_t>::iterator n1i = neighbors.begin(); n1i != neighbors.end(); ++n1i ) {
			// Construct vector of that person's neighbors
			// (those that are in the set of neighbors we're given)
		       	n1_neighbors.clear();
			construct_neighbors_neighbors( *n1i, neighbors, n, n1_neighbors, state, indent + "  " );

			// Get the sum_of_influences log likelihood for
			// neighbor's neighbors, recursively.
			float n1_sum_log_odds = 
				sum_of_influences(n1_neighbors, *n1i, n, params, state, indent + "  ");
			LOG_OUT << indent << "    sum of log_odds: " << n1_sum_log_odds << "\n";

			// given the neighbor's influences and their actual 
			// action, there are cases giving us this neighbor's
			// contribution to the log odds.
			float log_alpha_n1;
			int action = (state[*n1i].adopted ? 1 : -1);
			if (n1_sum_log_odds * action > rho) {
				// if the influence is enough to override their signal, we don't
				// know their signal because they were part of a cascade
				log_alpha_n1 = 0;
			} else if (n1_sum_log_odds * action == rho) {
				// if the influence is ±rho and they agreed with it, they might have
				// flipped a coin
				log_alpha_n1 = action * sigma;
			} else {
				// otherwise the influence is too weak and we're seeing their signal.
				// this includes the "paradoxical" case.
				log_alpha_n1 = action * rho;
			}
			LOG_OUT << indent << "  log alpha (" << n1 << "): " << log_alpha_n1 << "\n";
			// We simply add that to the total log odds.
			total_log_odds += log_alpha_n1;
		}
		if (memoize_sum_of_influences_p) {
			(*memoize_sum_of_influences_p)[n0] = total_log_odds;
		}
		return total_log_odds;
	}

	virtual void figure_out_update(vertex_index_t i, bool signal,
		network_t &n, params_t &params, state_container_t &state) { 
		// reconstruct everyone's decisions and try to infer their signals
		LOG_OUT << "Update site " << i << " (signal " << signal << "):\n";

		// get everyone in the neighborhood, from first to play, to last
		typename graph_traits<network_t>::adjacency_iterator ai,aend;
		set<vertex_index_t> neighbors;
		unsigned n_decided = 0, n_adopted = 0;
		for (tie(ai,aend) = adjacent_vertices(i, n); ai != aend; ++ai) {
		       	vertex_index_t j = *ai;
			if (state[j].decided) {
			       	++n_decided;
				if (state[j].adopted) {
					++n_adopted;
				}
				neighbors.insert(j);
			}
		}
		unsigned n_up = 0, n_down = 0;
		//sort( neighbors.begin(), neighbors.end(), compare_times(state) );

		if ( memoize_sum_of_influences_p == NULL ) {
			static string igt = "";
			if (igt == "") {
				igt = params.initial_graph_type(); 
			}
			if (igt == "COMPLETE") {
				memoize_sum_of_influences_p = new map<vertex_index_t,float>;
			}
		}
		// what are all the neighbors' signals worth to me?
		float total_log_alphas = sum_of_influences(neighbors, i, n, params, state, "");

		// given the total influence and the signal, decide what to do
		float rho_i = (signal ? rho : -rho);
		LOG_OUT << "  rho_i = " << rho_i << "\n";
		LOG_OUT << "  sum of log odds = " << (total_log_alphas + rho_i) << ": ";
		if (total_log_alphas + rho_i > 0) {
		       	state[i].adopted = true;
			LOG_OUT << 1;
			if (total_log_alphas > rho) {
			       	state[i].cascaded = true;
				LOG_OUT << " (cascade)";
			}
		} else if (total_log_alphas + rho_i < 0) {
		       	state[i].adopted = false;
			LOG_OUT << -1;
			if (total_log_alphas < -rho) {
			       	state[i].cascaded = true;
				LOG_OUT << " (cascade)";
			}
		} else {
		       	state[i].adopted = bernoulli_distribution<>(0.5)(this->rng);
			LOG_OUT << "coin flip: " << (state[i].adopted ? 1 : -1);
			state[i].flipped = true;
		}
		LOG_OUT << "\n";
		state[i].neighbors_adopted = n_adopted;
		state[i].neighbors_decided = n_decided;
	}
};

// same_neighborhood_log_odds_update_rule is an approximation to the above
// bikh_log_odds rule, in which a player interprets a neighbor's action as
// if the neighbor had access to all the player's neighbors, not just the ones
// the neighbor is connected to.  This should bring the combinatorics under
// control.  We think it's a conservative approximation, that's likely to make
// the model population less powerful, not more, because it makes these players
// less accurate.
template<typename network_t, typename params_t, typename rng_t>
class same_neighborhood_log_odds_update_rule : 
	public bikh_log_odds_update_rule<network_t,params_t,rng_t> {
public:
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::vertex_index_t 
		vertex_index_t;
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::state_container_t 
		state_container_t;
	typedef typename
	 	bikh_on_network_update_rule<network_t,params_t,rng_t>::compare_times
		compare_times;

	same_neighborhood_log_odds_update_rule(double p, rng_t &__rng) :
		bikh_log_odds_update_rule<network_t,params_t,rng_t>(p, __rng) {}

	// unlike the standard construct_neighbors_neighbors, which constructs the
	// intersection of neighborhood(n1) and neighborhood(n2), here we just say
	// n2's neighborhood is everyone in neighborhood(n1) who played before n2 did.
	// We also use memoize to let n1 reuse the inferences that n2 generates, which
	// allows us to save a whole lot of time.
	virtual void construct_neighbors_neighbors( vertex_index_t n1,
			set<vertex_index_t> &all_neighbors,
			network_t &n,
			set<vertex_index_t> &n1_neighbors,
			state_container_t &state,
		       	string indent) {
		LOG_OUT << indent << "  construct neighbors of " << n1 << ':';
		for ( typename set<vertex_index_t>::iterator n2i = all_neighbors.begin(); 
		      n2i != all_neighbors.end(); ++n2i ) {
			vertex_index_t n2 = *n2i;
			if (state[n2].decided && 
			    state[n2].time_of_decision < state[n1].time_of_decision) {
				n1_neighbors.insert(n2);
				LOG_OUT << ' ' << n2;
			} else {
				LOG_OUT << " (" << n2 << ')';
			}
		}
		LOG_OUT << endl;
	}
	
	using bikh_log_odds_update_rule<network_t,params_t,rng_t>::memoize_sum_of_influences_p;

	virtual void figure_out_update(vertex_index_t i, bool signal,
		network_t &n, params_t &params, state_container_t &state) { 
		if ( memoize_sum_of_influences_p == NULL ) {
			memoize_sum_of_influences_p = new map<vertex_index_t,float>;
		}
		memoize_sum_of_influences_p->clear();
		bikh_log_odds_update_rule<network_t,params_t,rng_t>::figure_out_update(
			i, signal, n, params, state);
	}
};

// counting_closure_log_odds_update_rule is another approximation to the
// full bikh_log_odds, in which we do the full neighborhood-intersection
// inference up to n levels of recursion, and on the nth recursion we suppose
// that that person actually used the counting rule instead of recursing
// any farther.
template<typename network_t, typename params_t, typename rng_t>
class counting_closure_log_odds_update_rule : 
	public bikh_log_odds_update_rule<network_t,params_t,rng_t> {
public:
	unsigned int close_level, level;

	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::state_container_t 
		state_container_t;
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::vertex_index_t 
		vertex_index_t;

	counting_closure_log_odds_update_rule(double p, rng_t &__rng) :
		bikh_log_odds_update_rule<network_t,params_t,rng_t>(p, __rng),
		close_level(-1), level(0) {}

	using bikh_log_odds_update_rule<network_t,params_t,rng_t>::rho;

	virtual float sum_of_influences(set<vertex_index_t>&neighbors, 
			vertex_index_t n0,
			network_t &n, params_t &params, 
			state_container_t &state, string indent = "") { 
		if ( close_level == (unsigned int)-1 ) {
			close_level = params.inference_closure_level();
		}
		if ( level < close_level ) {
			//LOG_OUT << indent << "  " << level << " < " << close_level << endl;
			++level;
			float soi = bikh_log_odds_update_rule<network_t,params_t,rng_t>::
				sum_of_influences( neighbors, n0, n, params, state, indent );
			--level;
			return soi;
		}
		// count the neighbors
		typename set<vertex_index_t>::iterator ji;
		unsigned n_decided = 0, n_adopted = 0;
		for (ji = neighbors.begin(); ji != neighbors.end(); ++ji) {
		       	vertex_index_t j = *ji;
			if (state[j].decided) {
			       	++n_decided;
				if (state[j].adopted) {
					++n_adopted;
				}
			}
		}
		int n_up = n_adopted, n_down = n_decided - n_adopted;
		LOG_OUT << indent << "closure: " << n_up << " up, "
			<< n_down << " down: " << (n_up - n_down) * rho << endl;
		// convert to the log-odds that the outer sum_of_influences
		// wants: (adoptions - rejections) times rho.
		return (n_up - n_down) * rho;
	}
};

template<typename network_t, typename params_t>
class network_dynamics {
public:
	network_dynamics(network_t &n0, params_t &p0) {}
	void step(void) {}
};

template<typename network_t, typename params_t, 
	typename update_rule_t, typename rng_t>
class update_everyone_once_dynamics : 
	public network_dynamics<network_t,params_t> {
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
			network_dynamics<network_t,params_t>(_n,_params)	{ 
		typename graph_traits<network_t>::vertex_iterator si,send;
		tie(si,send) = vertices(n);
		copy(si, send, inserter(yet_to_update, yet_to_update.begin()));
		random_shuffle(yet_to_update.begin(), yet_to_update.end());
	}
	
	double t() { return _t; }
	state_container_t &state() { return _state; }

	void step() {
	       	if (int(_t) % 100 == 0) {
			cout << _t << "\n";
		}
		vertex_index_t i = yet_to_update.front();
		yet_to_update.pop_front();
		already_updated.push_back(i);
		updater.update(i, _t, n, params, _state);
		_t += 1;
	}
};

template<typename network_t, typename state_t>
class color_cascade_state : public color_vertices<network_t> {
public:
	typedef typename color_vertices<network_t>::key_type key_type;
	typedef typename color_vertices<network_t>::value_type value_type;
	using color_vertices<network_t>::n;
	state_t &state;

	color_cascade_state(network_t&_n, state_t&_s) : 
		color_vertices<network_t>(_n), state(_s) {}
	value_type operator[](const key_type &e) const {
	       	return (*this)(e,n); 
	}

	value_type operator()(const key_type &e, const network_t&c) const {
	       	if (!state[e].decided) {
			return "white";
		} else if (!state[e].adopted && !state[e].cascaded) {
			return "red";
		} else if (!state[e].adopted && state[e].cascaded) {
			return "pink";
		} else if (state[e].adopted && !state[e].cascaded) {
			return "blue";
		} else if (state[e].adopted && state[e].cascaded) {
			return "lightblue";
		}
	}
};

template<typename network_t, typename state_t>
color_cascade_state<network_t, state_t> 
	make_color_cascade_state(network_t&n, state_t &s) {
	return color_cascade_state<network_t,state_t>(n,s);
}

