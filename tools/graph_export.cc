#include <fstream>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/serialization/traits.hpp>
#include <boost/serialization/version.hpp>

#include "marocco/BioGraph.h"
#include "marocco/coordinates/BioNeuron.h"
#include "marocco/results/Marocco.h"
#include "marocco/routing/util.h"

using namespace boost::program_options;

// fwd decl functions for later usage
bool is_connected(
    const marocco::BioNeuron& bio_src,
    const marocco::BioNeuron& bio_tgt,
    marocco::BioGraph::graph_type const& bio_graph);

std::vector<marocco::BioNeuron> getTargetNeurons(
    marocco::BioNeuron const& bio, marocco::BioGraph::graph_type const& bio_graph);

size_t verbosity = 0;

int main(const int argc, const char** argv)
{
	options_description desc{"Options"};
	desc.add_options()("load,l", value<std::string>(), "result to load")(
	    "out,o", value<std::string>(), "file to export the neuron graph to")(
	    "verbose,v", value<size_t>(), "set verbosity level");
	variables_map vm;
	store(parse_command_line(argc, argv, desc), vm);
	notify(vm);

	if (!vm.count("load") || !vm.count("out")) {
		std::cout << desc << '\n';
		return 2;
	}

	auto const input = vm["load"].as<std::string>();
	auto const output = vm["out"].as<std::string>();

	if (vm.count("verbose")) {
		verbosity = vm["verbose"].as<size_t>();
	}

	if (verbosity >= 1) {
		std::cout << "loading file <- " << input << '\n';
		std::cout << "exporting dot graph to -> " << output << '\n';
	}

	// load result
	marocco::results::Marocco result;
	try {
		result = marocco::results::Marocco::from_file(input);
	} catch (const std::exception& e) {
		std::cout << "\n\nTHE RESULTS FILE SEEMS CORRUPTED\n\n";
		std::cout << "error: " << e.what() << '\n';
		return 4;
	}

	// build nrn_graph
	if (verbosity >= 1) {
		std::cout << "reading the graph from " << input << " now\n";
	}
	auto graph = result.bio_graph;

	// export nrn_graph
	std::ofstream out(output, std::ofstream::out);

	if (!out.is_open()) {
		return EXIT_FAILURE;
	}
	out << "digraph NRN {" << '\n';

	auto v = boost::vertices(graph);

	if (verbosity >= 2) {
		std::cout << "iterating over : " << std::distance(v.first, v.second) << '\n';
	}

	for (auto vit = v.first; vit != v.second; vit++) {
		if (verbosity >= 3) {
			std::cout << "v: " << *vit << '\n';
		}
		Population const& pop = *((graph)[*vit]);
		if (verbosity >= 3) {
			std::cout << "pop.size: " << pop.size() << '\n';
		}
		for (size_t n = 0; n < pop.size(); ++n) {
			marocco::BioNeuron const nrn = marocco::BioNeuron(*vit, n);
			for (auto target_nrn : getTargetNeurons(nrn, graph)) {
				std::string const source_pop_label = graph[nrn.population()]->label() != ""
				                                         ? graph[nrn.population()]->label()
				                                         : std::to_string(nrn.population());
				std::string const target_pop_label = graph[target_nrn.population()]->label() != ""
				                                         ? graph[target_nrn.population()]->label()
				                                         : std::to_string(target_nrn.population());
				out << "\"" << source_pop_label << "[" << nrn.neuron_index() << "]\""
				    << " -> "
				    << "\"" << target_pop_label << "[" << target_nrn.neuron_index() << "]\""
				    << '\n';
			}
		}
	}
	out << "}" << '\n';
	out.close();

	return EXIT_SUCCESS;
}

bool is_connected(
    const marocco::BioNeuron& bio_src,
    const marocco::BioNeuron& bio_tgt,
    marocco::BioGraph::graph_type const& bio_graph)
{
	auto const& outs = marocco::make_iterable(out_edges(bio_src.population(), bio_graph));

	for (auto const& edge : outs) {
		if (bio_tgt.population() != boost::target(edge, bio_graph)) {
			// the target neurons population is not the target of this edge
			continue;
		}

		ProjectionView const proj_view = (bio_graph)[edge]; // get the projection for this edge
		if (!proj_view.pre().mask()[bio_src.neuron_index()]) {
			// does the source bio neuron want to realise a connection on this edge???
			continue;
		}

		if (!proj_view.post().mask()[bio_tgt.neuron_index()]) {
			// does the target bio neuron has a connection to realise this edge???
			continue;
		}

		Connector::const_matrix_view_type const bio_weights = proj_view.getWeights();

		size_t const src_neuron_in_proj_view =
		    marocco::routing::to_relative_index(proj_view.pre().mask(), bio_src.neuron_index());
		size_t const trg_neuron_in_proj_view =
		    marocco::routing::to_relative_index(proj_view.post().mask(), bio_tgt.neuron_index());

		double const weight = bio_weights(src_neuron_in_proj_view, trg_neuron_in_proj_view);

		if (std::isnan(weight) || weight <= 0.) {
			// is there a weight to realise
			continue;
		}

		// all checks passed;
		return true;
	}
	return false;
}

std::vector<marocco::BioNeuron> getTargetNeurons(
    marocco::BioNeuron const& bio, marocco::BioGraph::graph_type const& bio_graph)
{
	std::vector<marocco::BioNeuron> result;

	auto const& edges_out = marocco::make_iterable(out_edges(bio.population(), bio_graph));

	for (auto const& edge : edges_out) {
		auto const target_vertex = boost::target(edge, bio_graph);
		Population const& target_pop = *((bio_graph)[target_vertex]);

		for (size_t n = 0; n < target_pop.size(); ++n) {
			marocco::BioNeuron const bio_tgt = marocco::BioNeuron(target_vertex, n);
			if (is_connected(bio, bio_tgt, bio_graph)) {
				result.push_back(bio_tgt);
			}
		}
	}
	return result;
}
