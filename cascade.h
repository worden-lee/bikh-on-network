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

#if 0
#define LOG_OUT cout
#else
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/null.hpp"
boost::iostreams::stream< boost::iostreams::null_sink > nullOstream( ( boost::iostreams::null_sink() ) );
#define LOG_OUT nullOstream
#endif

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

	float sum_of_influences(vector<vertex_index_t>&neighbors, 
			network_t &n, state_container_t &state, string indent = "")
	{ 
		// in order, try to infer their signal from what they were looking at
		float total_influence = 0;
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
			float n1_sum_influence = 
				sum_of_influences(n1_neighbors, n, state, indent + "  ");
			LOG_OUT << indent << "    sum of influences: " << n1_sum_influence << "\n";

			// given the sum_influence and their actual action, there are cases.
			float n1_inferred_signal;
			int action = (state[neighbors[n1]].adopted ? 1 : -1);
			// if the influence is enough to override their signal, we don't
			// know their signal because they were part of a cascade
			if (n1_sum_influence * action > 1)
				n1_inferred_signal = 0;
			// if the influence is ±1 and they agreed with it, they might have
			// flipped a coin
			else if (n1_sum_influence * action == 1)
				n1_inferred_signal = action / 3.0;
			// otherwise the influence is too weak and we're seeing their signal.
			else
				n1_inferred_signal = action;
			LOG_OUT << indent << "    inferred signal: " << n1_inferred_signal << "\n";
			total_influence += n1_inferred_signal;
		}
		return total_influence;
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
		float total_influence = sum_of_influences(neighbors, n, state);

		// given the total influence and the signal, decide what to do
		int signed_signal = (signal ? 1 : -1);
		LOG_OUT << "  total influence = " << total_influence << ": ";
		if (total_influence + signed_signal > 0)
		{ state[i].adopted = true;
			LOG_OUT << 1;
			if (total_influence > 1)
			{ state[i].cascaded = true;
				LOG_OUT << " (cascade)";
			}
		}
		else if (total_influence + signed_signal < 0)
		{ state[i].adopted = false;
			LOG_OUT << -1;
			if (total_influence < -1)
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


template<typename network_t, typename params_t, typename rng_t>
class bayesian_with_horizon_update_rule : 
	public bikh_on_network_update_rule<network_t,params_t,rng_t>
{
public:
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::state_container_t 
		state_container_t;
	typedef typename 
		bikh_on_network_update_rule<network_t,params_t,rng_t>::vertex_index_t 
		vertex_index_t;
	using bikh_on_network_update_rule<network_t,params_t,rng_t>::p;
	typedef typename
	 	bikh_on_network_update_rule<network_t,params_t,rng_t>::compare_times
		compare_times;

	bayesian_with_horizon_update_rule(double p, rng_t &__rng) :
		bikh_on_network_update_rule<network_t,params_t,rng_t>(p,__rng) {}

	// the posterior likelihood that true signal is 1, given an "excess
	// signal" of k = sum_i (2x_i - 1).
	float s(int k) 
	{ return 1.0 / (pow((1-p)/p, k) + 1);
	}

	// the posterior likelihood that true signal is 1, given a list of
	// "action probability" pairs for predecessors players, and my signal,
	// for both possible values of my signal.
	// this calls itself recursively with information accumulating in 
	// the "excess signal" k.  Call it from outside with k = 0.
  vector<float>
	 	double_V(vector< vector<float> > &action_probabilities,
						 vector<unsigned> &predecessors, int upto = -2, int k = 0)
	{ if (predecessors.empty() || upto == -1)
		{ vector<float> sk(2);
		  sk[0] = s(k-1);
		  sk[1] = s(k+1);
			LOG_OUT << '(' << sk[0] << ", " << sk[1] << ')';
			return sk;
		}
		else
		{ if (upto < 0) upto = predecessors.size() - 1;
			vector<float> accum(2,0);
			LOG_OUT << '(';
		  if (action_probabilities[upto][0] < 1)
				LOG_OUT << action_probabilities[upto][0];
			if (action_probabilities[upto][0] > 0)
			{ LOG_OUT << ' ';
				vector<float> vs = 
					double_V(action_probabilities, predecessors, upto-1, k-1);
				accum[0] += action_probabilities[upto][0] * vs[0];
				accum[1] += action_probabilities[upto][0] * vs[1];
			}
			LOG_OUT << " +";
			if (action_probabilities[upto][1] < 1) 
			  LOG_OUT << ' ' << action_probabilities[upto][1];
			if (action_probabilities[upto][1] > 0)
			{ LOG_OUT << ' ';
				vector<float> vs = 
					double_V(action_probabilities, predecessors, upto-1, k+1);
				accum[0] += action_probabilities[upto][1] * vs[0];
				accum[1] += action_probabilities[upto][1] * vs[1];
			}
			LOG_OUT << ')';
			return accum;
		}
	}
	
	// Action probability function: probability that player takes action
	// a (= 0 or 1), given that their likelihood that true signal is up is v.
	float A(int a, float v)
	{ float A1;
		if (v > 0.5)
			A1 = 1;
		else if (v == 0.5)
			A1 = 0.5;
		else
			A1 = 0;
		if (a == 0)
			return 1 - A1;
		else
			return A1;
	}

	// as below, but converted to likelihoods, i.e. instead of
	// a_i we use a_i / (a_0 + a_1).
	vector<float> action_likelihoods(bool action,
			vector<vertex_index_t> &neighbors,
			network_t &n, state_container_t &state, string indent = "")
	{ vector<float> as = action_probabilities(action,
			neighbors, n, state, indent);
		float sum = as[0] + as[1];
		if (sum > 0) // the sane case
		{ as[0] /= sum;
			as[1] /= sum;
		}
		else
	  { // need to also handle the insane case, though:
			// where there's no signal that could account for the action the
			// player took, because they appear to be in a contrary cascade.
			// This can happen because of influences outside our view.  In this
			// case we say our player doesn't really know what happened, but
			// assumes they wouldn't have taken such a contrary action without
			// a signal to match.
			as[action] = 1;
		}
		LOG_OUT << " -> (" << as[0] << ", " << as[1] << ")\n";
		return as;
	}

	// given that these are the neighbors you can see (who have played
	// before you), the probability that you take the action mentioned, 
	// given that your signal is down and given that your signal is up, 
	// in that order.
	vector<float> action_probabilities(bool action,
			vector<vertex_index_t> &neighbors,
			network_t &n, state_container_t &state, string indent = "")
	{ // reconstruct everyone's decisions before me
		vector< vector<float> > neighbor_as( neighbors.size() );

		// for each neighbor n1, try to infer their signal from 
		// what they were looking at and what they did
		vector<unsigned> n1_neighbors; // temp list of neighbor's neighbors
		for ( unsigned n1 = 0; n1 < neighbors.size(); ++n1 )
		{ 
			// which players were they looking at?
			n1_neighbors.clear();
			LOG_OUT << indent << "  neighbor " << neighbors[n1] 
				<< " (action " << state[neighbors[n1]].adopted << ") sees [";
			for ( unsigned n2 = 0; n2 < n1; ++n2 )
			{ typename boost::graph_traits<network_t>::edge_descriptor e;
				bool edge_exists;
				tie(e, edge_exists) = edge(neighbors[n1],neighbors[n2],n);
				if (edge_exists)
				{ // player n2 was visible to player n1
					n1_neighbors.push_back(neighbors[n2]);
					LOG_OUT << ' ' << neighbors[n2];
				}
			}
			LOG_OUT << " ]\n";

			// what could they have done with that information?
			// Do the likelihood calculation and get the two probabilities:
			// P(taking the action they did | an up signal)
			// P(taking the action they did | a down signal)
			neighbor_as[n1] = action_likelihoods(state[neighbors[n1]].adopted,
					n1_neighbors, n, state, indent + "  ");
		}
		
		// now given the A values for each neighbor, do the recursive calculation
		// for likelihood that true value is high.
		LOG_OUT << indent << "  ";
		vector<float> vs = double_V(neighbor_as, neighbors);
		vector<float> as(2);
		as[0] = A(action, vs[0]);
		as[1] = A(action, vs[1]);
	  LOG_OUT << " = (" << vs[0] << ", " << vs[1] << ") => (" 
			<< as[0] << ", " << as[1] << ")";
		return as;
	}

	virtual void figure_out_update(vertex_index_t i, bool signal,
		 network_t &n, state_container_t &state)
	{ LOG_OUT << "Update site " << i << " (signal " << signal << "):\n";

		// list everyone in the neighborhood, from first player to last
		vector<vertex_index_t> neighbors;
		unsigned n_decided = 0, n_adopted = 0;
		typename graph_traits<network_t>::adjacency_iterator ai,aend;
		for (tie(ai,aend) = adjacent_vertices(i, n); ai != aend; ++ai)
		{ vertex_index_t j = *ai;
			if (state[j].decided)
			{ neighbors.push_back(j);
				++n_decided;
				if (state[j].adopted)
					++n_adopted;
			}
		}
		sort( neighbors.begin(), neighbors.end(), compare_times(state) );

		// get the probabilities of adopting depending on my signal
		vector<float> as = action_probabilities(1, neighbors, n, state);
		LOG_OUT << '\n';

		// what matters is the answer for my actual signal.
		float A_i = as[signal];
		// Adopt if it's > 0.5, flip a coin if = 0.5.
		if (A_i > 0.5)
		{ state[i].adopted = 1;
			LOG_OUT << "  Action: 1";
			float A_counterfactual = as[!signal];
			if (A_counterfactual > 0.5)
			{ state[i].cascaded = 1;
				LOG_OUT << " in cascade";
			}
			LOG_OUT << ".\n";
		}
		else if (A_i == 0.5)
		{ state[i].adopted = bernoulli_distribution<>(0.5)(this->rng);
			LOG_OUT << "  Coin flip: " << (state[i].adopted ? 1 : 0) << ".\n";
			state[i].flipped = true;
		}
		else
		{	state[i].adopted = 0;
			LOG_OUT << "  Action: 0";
			float A_counterfactual = as[!signal];
			if (A_counterfactual < 0.5)
			{	state[i].cascaded = 1;
				LOG_OUT << " in cascade";
			}
			LOG_OUT << ".\n";
		}

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

