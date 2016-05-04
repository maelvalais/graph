/*
 * stable_marriage.hpp
 *
 *  Created on: Jun 8, 2015
 *      Author: maelv
 *
 *  From Wikipedia: http://en.wikipedia.org/wiki/Stable_marriage_problem
 *
 *  Simple explanation
 *  ==================
 *  Imagine you have three men and three women to marry; you would want them
 *  to be the happiest. You ask to the six of them their preferences (they
 *  order their potential partners). Then, you match them according to those
 *  preferences, and optimizing their happiness.
 *
 *  Wikipedia
 *  =========
 *  The stable marriage problem (also stable matching problem or SMP) is the
 *  problem of finding a stable matching between two equally sized sets of
 *  elements given an ordering of preferences for each element.
 *
 *  See the example at libs/graph/example/stable_marriage_example.cpp.
 *
 */

#ifndef STABLE_MARRIAGE_HPP_
#define STABLE_MARRIAGE_HPP_
#include <boost/graph/bipartite.hpp>
#include <boost/graph/graph_traits.hpp>
#include <vector>

/**
 * Complexity: 0((m+n)^2), with m and n being the size of the two sets
 *
 * g (in/out)
 * The directed graph. The graph must be directed because of the direction
 * of preferences. A man from one set can have a stronger preference for a
 * woman than that woman for that man (and vice-versa).
 * I call an edge a "relation", meaning that a man may want to have a relation
 * (= get engaged) with a woman, but the opposite can be false (= the opposite
 * edge may not exist).
 *
 * engaged_to_map (out)
 * This edge property map stores the engagement state of a person who belongs
 * to one group with another person of the opposite group. If a person of the
 * group WHITE is engaged to a person of the group BLACK, then the engaged_to
 * property must be the same in the other way around.
 * The value_type of this property map must be a model of the concept of
 * ColorValue.
 * The key_type must is the same as the edge descriptor type of the graph.
 * The property map must be a model of the Read/Write Property Map concept.
 *
 * preference_map (in)
 * This edge property map gives to the algorithm a way to know of much a
 * person from on group prefers a person from the opposite group. You don't
 * need to have reciprocity (i.e. every edge doesn't need to exist in the
 * graph). This map must go along with the `compare` function.
 * The value_type must be boolean.
 * The key_type must is the same as the vertex descriptor type of the graph.
 * The property map must be a model of the Readable Property Map concept.
 *
 * compare (in)
 * This function (you must pass a functor -- a pointer to a function) allows
 * the algorithm to compare to preferences.
 * The function must be a model of the concept Binary Function. The function
 * must return true if the left argument is better (in term of "preferences")
 * than the right argument. The argument types are the same as the
 * preference_map value_type.
 *     Example: std::greater_than<T>()
 *     with T = property_traits<PreferenceMap>::value_type.
 *
 * vertex_index_map (in)
 * This vertex property map maps every vertex (using the graph's vertex
 * descriptor) to an integer in the range [0,num_vertices(g)).
 * It is necessary to the algorithm that finds the two partitions of the
 * bipartite graph.
 *     Example: get(boost::vertex_index,g)
 *
 * color_map (out)
 * This vertex property map stores the group (BLACK of WHITE) a vertex
 * belongs to. This property is computed by is_bipartite().
 * NOTE: I often identify the BLACK color group as the "men" group, but it is
 * totally arbitrary.
 *     Example: std::vector<boost::default_color_type>
 *
 * I used these words to mean:
 *     - relation: any edge in the graph is a relation ; when I say relation,
 *     I mean a one-side relation. So if a man and a woman actually get
 *     engaged, it means that both relations man->woman and woman->man will
 *     be effective (= the engaged_to edge property will be set)
 */
template < typename Graph, typename PreferenceMap, typename CompareFunction,
    typename EngagedToMap, typename VertexIndexMap, typename ColorMap >
void stable_marriage(Graph& g, EngagedToMap engaged_to_map,
    PreferenceMap preference_map, CompareFunction compare,
    VertexIndexMap vertex_index_map, ColorMap color_map)
{
    // First, we reconstitute the two sets that constitute our bipartite graph
    // We consider men as the boost::color_black vertices and women as
    // boost::color_white vertices
    is_bipartite(g, vertex_index_map, color_map);

    typedef typename boost::graph_traits< Graph >::edge_descriptor relation_t;
    typedef typename boost::graph_traits< Graph >::edge_iterator
        all_relations_iterator_t;
    typedef typename boost::graph_traits< Graph >::out_edge_iterator
        person_relation_iterator_t;
    typedef typename boost::graph_traits< Graph >::vertex_descriptor person_t;
    typedef typename boost::graph_traits< Graph >::vertex_iterator
        all_people_iterator_t;

    // Initialize every man/woman as free
    std::vector< bool > free_vector(boost::num_vertices(g), true);
    boost::iterator_property_map< std::vector< bool >::iterator, VertexIndexMap >
        free_map
        = make_iterator_property_map(free_vector.begin(), vertex_index_map);

    // Initialize the has_proposed_map and the engaged_to_map
    // How `has_proposed_map` is used here: for the edge (man,woman),
    // has_proposed_map[edge] is true if the man has already proposed
    // to the woman to get engaged.
    // NOTE: this property is only used for edges from one the men group
    // to the women group, and never for the other way around
    std::map< relation_t, bool > has_proposed_internal_map;
    boost::associative_property_map< std::map< relation_t, bool > >
        has_proposed_map(has_proposed_internal_map);

    { // dummy block to re-use first and last variables
        all_relations_iterator_t first, last;
        tie(first, last) = edges(g);
        for (; first != last; ++first)
        {
            engaged_to_map[*first] = false;
            has_proposed_internal_map.insert(std::make_pair(*first, false));
        }
    }
    // We only stop if every man has proposed to every woman ;
    // if we see a man having proposed to every woman (= not man_may_propose)
    // then we turn that man into "not free" mode (even if this mode should be
    // reserved to the "not engaged" mode)
    bool every_man_is_engaged = false;
    all_people_iterator_t men_iterator, men_iterator_end;
    boost::tie(men_iterator, men_iterator_end) = boost::vertices(g);
    while (!every_man_is_engaged)
    {

        // Look for the next man not engaged (= free)
        while (men_iterator != men_iterator_end
            && (!(color_map[*men_iterator] == boost::black_color)
                   || !free_map[*men_iterator]))
        {
            men_iterator++;
        }

        if (men_iterator == men_iterator_end)
        {
            every_man_is_engaged = true;
        }
        else
        {

            // Now that we know that a man is free, we can look for the best
        	// woman (that he did not proposed to engage yet) using the
        	// preference_map
            person_t current_prefered_woman;
            relation_t current_prefered_relation;
            double current_preference = 0;
            bool man_may_propose = false;
            person_relation_iterator_t relation_it, relation_it_end;
            tie(relation_it, relation_it_end)
                = boost::out_edges(*men_iterator, g);
            for (; relation_it != relation_it_end; relation_it++)
            {

                person_t current_woman = target(*relation_it, g);

                if (!has_proposed_map[*relation_it] // this man hasn't proposed
                                                    // to that
                    // woman yet
                    && edge(current_woman, *men_iterator, g)
                           .second // the woman can engage with that man
                    && compare(
                           preference_map[*relation_it], current_preference))
                { // the man prefers even more that woman

                    current_preference = preference_map[*relation_it];
                    current_prefered_woman = current_woman;
                    current_prefered_relation = *relation_it;
                    man_may_propose = true;
                }
            }

            // Now that we found the best woman for that man, let's check if
            // she is free and if she is not, let's ask her if she prefers this
            // new man instead of her previous one
            if (man_may_propose)
            {

                has_proposed_map[current_prefered_relation] = true;
                if (free_map[current_prefered_woman])
                {
                    // This woman is free: lets get engaged!
                    engaged_to_map[current_prefered_relation] = true;
                    engaged_to_map[edge(current_prefered_woman, *men_iterator,
                        g).first]
                        = true;
                    free_map[current_prefered_woman] = false;
                    free_map[*men_iterator] = false;
                }
                else
                {
                    // This woman is not free: we must ask her wether this new
                    // man is
                    // more engaging than the previous one

                    // First, look for the man she is currently engaged to
                    person_t prev_man;
                    person_relation_iterator_t first, last;
                    boost::tie(first, last)
                        = boost::out_edges(current_prefered_woman, g);
                    for (; first != last && !engaged_to_map[*first]; first++)
                    {
                    }
                    prev_man = target(*first, g);

                    // Then, we can check if the new man is better than the
                    // previous one
                    if (preference_map[edge(current_prefered_woman,
                            *men_iterator,
                            g).first]
                        > preference_map[edge(current_prefered_woman, prev_man,
                              g).first])
                    {
                        engaged_to_map[edge(*men_iterator,
                            current_prefered_woman,
                            g).first]
                            = true;
                        engaged_to_map[edge(current_prefered_woman,
                            *men_iterator,
                            g).first]
                            = true;
                        free_map[current_prefered_woman] = false;
                        free_map[*men_iterator] = false;

                        engaged_to_map[boost::edge(current_prefered_woman,
                            prev_man, g).first]
                            = false;
                        engaged_to_map[edge(prev_man, current_prefered_woman,
                            g).first]
                            = false;
                        free_map[prev_man] = true;
                    }
                }
            }
            else
            {
                // This man could not propose to any woman: we must
                // remember it to avoid re-analyzing this man
                free_map[*men_iterator] = false;
            }
        }
    }
}

// Useless as we actually use a vertex property map to store if a person
// is engaged.
template < typename Vertex, typename Graph, typename EngagedToMap >
std::pair< Vertex, bool > is_engaged(
    const Vertex v, const Graph g, EngagedToMap engaged_to_map)
{
    typename boost::graph_traits< Graph >::edge_iterator it, it_end;
    tie(it, it_end) = out_edges(v, g);
    for (; it != it_end; it++)
        if (engaged_to_map[*it])
            return std::pair< Vertex, bool >(target(*it, g), true);
    return std::pair< Vertex, bool >(Vertex(), true);
}

#endif /* STABLE_MARRIAGE_HPP_ */
