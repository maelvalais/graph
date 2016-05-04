/*
 * stable_marriage_example.cpp
 *
 *  Created on: Jun 11, 2015
 *      Author: Maël Valais
 *
 *  This example will match a group of three men and three women,
 *  printing in stdout the resulting stable marriage graph in dot format.
 *  To open it, use Graphviz
 *
 *  To compile: go to the libs/graph/example/ directory and then run
 *
 *      ../../../b2 cxxflags="-std=c++11" stable_marriage
 *
 *  The binary `stable_marriage_example` will be created somewhere in
 *  ../../../bin.v2/libs/graph/example/?/?/stable_marriage_example. You
 *  can launch it, and a dot formatted graph like that should appear:
 *  
 *  digraph G {
 *  0[label="0",color="pink"];
 *  1[label="1",color="pink"];
 *  2[label="2",color="pink"];
 *  ...
 *  }
 *
 *  Save it as graph.dot and open it with Graphviz!
 *
 */

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/stable_marriage.hpp>

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

int main(int argc, char **argv) {
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

    graph_t g;

    boost::property_map<graph_t, double edge_prop_t::*>::type map_preferences
        = boost::get(&edge_prop_t::preference,g);
    boost::property_map<graph_t, bool edge_prop_t::*>::type map_engaged_to
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
            map_engaged_to,
            map_preferences, 
            std::greater_equal<double>(),
            get(boost::vertex_index,g),
            color_map);

    boost::write_graphviz(std::cout, g,
            make_vertex_writer(color_map, get(boost::vertex_index,g)),
            make_edge_writer(map_engaged_to, map_preferences)
    );
}
