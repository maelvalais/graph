/*
 * stable_marriage_test.cpp
 *
 *  Created on: May 14, 2016
 *      Author: Maël Valais
 *
 *
 */
#define BOOST_TEST_MODULE stable_marriage_test

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/stable_marriage.hpp>
#include <boost/test/unit_test.hpp>

template <class EngagedToMap, class PreferenceMap>
class edge_writer {
public:
    edge_writer(EngagedToMap engaged_to, PreferenceMap pref_map) : engaged_to(engaged_to), pref_map(pref_map) {}
    template <class Edge>
    void operator()(std::ostream &out, const Edge& edge) const {
        out << "[color=\"" << ((engaged_to[edge])?"red":"black") << "\", label=\""<<pref_map[edge]<<"\"]";
    }
private:
    EngagedToMap engaged_to;
    PreferenceMap pref_map;
};
template <class EngagedToMap, class PreferenceMap>
inline edge_writer<EngagedToMap, PreferenceMap>
make_edge_writer(EngagedToMap engaged_to_map, PreferenceMap pref_map) {
    return edge_writer<EngagedToMap, PreferenceMap>(engaged_to_map, pref_map);
}


template <class ColorMap, class IndexMap>
class vertex_writer {
public:
    vertex_writer(ColorMap color_map, IndexMap index): color_map(color_map),index(index) {}
    template<class Vertex>
    void operator()(std::ostream &out, const Vertex& vertex) const {
        out << "[label=\""<< index[vertex] <<"\",color=\"" << (color_map[vertex]==boost::black_color?"blue":"pink") << "\"]";
    }
private:
    ColorMap color_map;
    IndexMap index;
};

template <class ColorMap, class IndexMap>
inline vertex_writer<ColorMap, IndexMap>
make_vertex_writer(ColorMap color_map, IndexMap index_map) {
    return vertex_writer<ColorMap, IndexMap>(color_map,index_map);
}

BOOST_AUTO_TEST_CASE(three_men_three_women) {
    // Create a small graph from a preference matrix
    int men_preferences[][3] = {{1,4,3},{2,5,2},{4,3,6}};
    int women_preferences[][3] = {{2,2,3},{4,3,5},{2,3,2}};
    struct vertex_prop_t {
        bool free;
        boost::default_color_type belonging; // the set (man of woman) this vertex belongs to
    };
    struct edge_prop_t {
        double preference;
        bool has_proposed;
        bool is_engaged;
    };
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, vertex_prop_t, edge_prop_t> graph_t;
    typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<graph_t>::edge_iterator edge_iterator_t;

    graph_t g;

    boost::property_map<graph_t, double edge_prop_t::*>::type map_preferences
        = boost::get(&edge_prop_t::preference,g);
    boost::property_map<graph_t, bool edge_prop_t::*>::type map_engaged
        = boost::get(&edge_prop_t::is_engaged,g);
    boost::property_map<graph_t, boost::default_color_type vertex_prop_t::*>::type color_map
        = boost::get(&vertex_prop_t::belonging,g);

    std::vector<vertex_t> men, women;
    for (int i = 0; i < 3; ++i) { // create men
        vertex_t new_man = boost::add_vertex(g);
        men.push_back(new_man);
    }
    for (int j = 0; j < 3; ++j) { // create women
        vertex_t new_woman = boost::add_vertex(g);
        women.push_back(new_woman);
    }

    // set preferences between men and women
    for (vertex_t man : men) {
        for (vertex_t woman : women) {
            map_preferences[boost::add_edge(man,woman,g).first] = men_preferences[man][woman-3];
            map_preferences[boost::add_edge(woman,man,g).first] = women_preferences[woman-3][man];
        }
    }

    stable_marriage(
            g,
            map_engaged,
            map_preferences,
            std::greater_equal<double>(),
            get(boost::vertex_index,g),
            color_map);

    // Engaged couples expected
    vertex_t mutually_engaged[][2] = {{0,4},{1,5},{2,3}};

    edge_iterator_t it, end;
    for(boost::tie(it,end) = boost::edges(g); it!=end; it++) {

    	// Check if this edge is one of the known mutually_engaged
    	bool engaged = false;
    	for(vertex_t* c : mutually_engaged) { // c for couple
    		if(*it == boost::edge(c[0],c[1],g).first) {
    			BOOST_CHECK(true == map_engaged[boost::edge(c[0],c[1],g).first]);
    			engaged = true;
    			break;
    		}
    		else if(*it == boost::edge(c[1],c[0],g).first) {
    		    BOOST_CHECK(true == map_engaged[boost::edge(c[1],c[0],g).first]);
    		    engaged = true;
    		    break;
    		}
    	}

    	if(!engaged) {
    		BOOST_CHECK(false == map_engaged[*it]); // Ex: {0,5} should not be engaged
    	}
    }
}
