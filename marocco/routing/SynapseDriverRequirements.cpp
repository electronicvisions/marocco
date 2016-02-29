#include "marocco/routing/SynapseDriverRequirements.h"
#include <tuple>

#include "hal/Coordinate/iter_all.h"
#include "marocco/routing/util.h"
#include "marocco/util/chunked.h"
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/tuple.h>
#include <boost/serialization/unordered_map.h>

using namespace HMF::Coordinate;
using HMF::HICANN::L1Address;
using HMF::HICANN::DriverDecoder;

namespace marocco {
namespace routing {

std::ostream& operator<<(std::ostream& os, Parity p)
{
	switch (p) {
		case Parity::even:
			os << "even";
			break;
		case Parity::odd:
			os << "odd";
			break;
		default:
			throw std::runtime_error("unsupported switch case");
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, TriParity p)
{
	switch (p) {
		case TriParity::even:
			os << "even";
			break;
		case TriParity::odd:
			os << "odd";
			break;
		case TriParity::any:
			os << "any";
			break;
		default:
			throw std::runtime_error("unsupported switch case");
	}
	return os;
}

SynapseDriverRequirements::SynapseDriverRequirements(
	HMF::Coordinate::HICANNGlobal const& hicann,
	marocco::placement::NeuronPlacementResult const& nrnpl,
	SynapseTargetMapping const& syn_tgt_mapping)
	: mHICANN(hicann),
	  mNeuronPlacement(nrnpl),
	  mSynapseTargetMapping(syn_tgt_mapping),
	  mNeuronWidth(extract_neuron_width(mNeuronPlacement.at(mHICANN))),
	  mTargetSynapsesPerSynapticInputGranularity(
		  calc_target_synapses_per_synaptic_input_granularity(mNeuronWidth, mSynapseTargetMapping))
{
}

std::map<Type_Decoder_STP, std::map<Side_Parity, size_t> >
SynapseDriverRequirements::count_half_rows_per_input_granularity(
	std::map<Type_Decoder_STP, size_t> const& synapses_per_property,
	std::map<SynapseType, std::map<Side_Parity, size_t> > const&
		target_synapses_per_parity_and_synaptic_input)
{
	std::map<Type_Decoder_STP, std::map<Side_Parity, size_t> > required_half_rows;

	for (auto bio_prop : synapses_per_property) {
		Type_Decoder_STP const& prop = bio_prop.first;
		const size_t synapse_count = bio_prop.second;
		auto it = target_synapses_per_parity_and_synaptic_input.find(std::get<0>(prop));
		auto it_cend = target_synapses_per_parity_and_synaptic_input.cend();
		if (it != it_cend) {
			for (auto config : it->second) {
				const size_t input_count = config.second;
				if (input_count > 0) {
					size_t const num_half_rows = std::ceil(synapse_count / double(input_count));
					required_half_rows[prop][config.first] = num_half_rows;
				}
			}
		}
	}

	return required_half_rows;
}

// helper function

template <typename Key>
size_t val_or_zero(Key const& key, std::map<Key, size_t> const& map)
{
	auto it = map.find(key);
	if (it != map.cend())
		return it->second;
	else
		return 0.;
}

std::pair<size_t, TriParity> SynapseDriverRequirements::get_min_realizing_count_and_parity(
	Side const& side, std::map<Side_Parity, size_t> const& map)
{
	size_t count = 0;
	TriParity parity;

	size_t c_even = val_or_zero<Side_Parity>(Side_Parity(side, Parity::even), map);
	size_t c_odd = val_or_zero<Side_Parity>(Side_Parity(side, Parity::odd), map);

	if (c_even > 0) {
		if (c_odd > 0) {
			if (c_even < c_odd) {
				count = c_even;
				parity = TriParity::even;
			} else if (c_odd < c_even) {
				count = c_odd;
				parity = TriParity::odd;
			} else { // equal
				count = c_even;
				parity = TriParity::any;
			}
		} else {
			count = c_even;
			parity = TriParity::even;
		}
	} else {
		if (c_odd > 0) {
			count = c_odd;
			parity = TriParity::odd;
		}
	}
	return {count, parity};
}

std::map<TriParity, std::map<Side_Decoder_STP, size_t> > SynapseDriverRequirements::count_half_rows(
	std::map<Type_Decoder_STP, std::map<Side_Parity, size_t> > const&
		half_rows_per_input_granularity,
	std::map<Type_Decoder_STP, Side_TriParity>& bio_to_hw_assignment)
{

	std::map<TriParity, std::map<Side_Decoder_STP, size_t> > rv;

	for (auto bio_prop : half_rows_per_input_granularity) {
		Type_Decoder_STP const& prop = bio_prop.first;
		auto const& input_granularity_count = bio_prop.second;

		TriParity parity_left, parity_right;
		size_t count_left, count_right;

		std::tie(count_left, parity_left) =
			get_min_realizing_count_and_parity(left, input_granularity_count);
		std::tie(count_right, parity_right) =
			get_min_realizing_count_and_parity(right, input_granularity_count);

		size_t count = 0;
		TriParity parity;
		Side side;

		if (count_left > 0) {
			if (count_right > 0) {
				// both have values
				// prefer the one with less required half rows
				if (count_left < count_right) {
					count = count_left;
					parity = parity_left;
					side = left;
				} else if (count_right < count_left) {
					count = count_right;
					parity = parity_right;
					side = right;
				} else {
					// equal counts on left and right
					// prefer Parity::any otherwise use the left input
					// TODO: or should we prefer even or odd parity?
					if (parity_left == TriParity::any) {
						count = count_left;
						parity = parity_left;
						side = left;
					} else if (parity_right == TriParity::any) {
						count = count_right;
						parity = parity_right;
						side = right;
					} else {
						count = count_left;
						parity = parity_left;
						side = left;
					}
				}
			}
			// only left input
			else {
				count = count_left;
				parity = parity_left;
				side = left;
			}
		} else if (count_right > 0) {
			// only right input
			count = count_right;
			parity = parity_right;
			side = right;
		}

		if (count > 0) {
			rv[parity][Side_Decoder_STP(side, std::get<1>(prop), std::get<2>(prop))] = count;
			bio_to_hw_assignment[prop] = Side_TriParity(side, parity);
		}
	}
	return rv;
}

std::map<TriParity, std::map<Side_Decoder_STP, size_t> >
SynapseDriverRequirements::count_half_rows_vec(
	std::vector<std::map<TriParity, std::map<Side_Decoder_STP, size_t> > > const&
		required_half_rows_per_neuron)
{
	std::map<TriParity, std::map<Side_Decoder_STP, size_t> > rv;

	for (auto nrn : required_half_rows_per_neuron) {
		for (auto p_and_counts : nrn) {
			TriParity parity = p_and_counts.first;
			std::map<Side_Decoder_STP, size_t>& global_counts = rv[parity];
			std::map<Side_Decoder_STP, size_t> const& counts = p_and_counts.second;
			for (auto item : counts) {
				auto key = item.first;
				const size_t count = item.second;
				std::map<Side_Decoder_STP, size_t>::iterator it = global_counts.find(key);
				if (it != global_counts.end())
					it->second = std::max(it->second, count);
				else
					global_counts[key] = count;
			}
		}
	}
	return rv;
}

std::map<TriParity, std::map<Side_STP, size_t> >
SynapseDriverRequirements::count_half_rows_per_side(
	std::map<TriParity, std::map<Side_Decoder_STP, size_t> > const& required_half_rows)
{
	std::map<TriParity, std::map<Side_STP, size_t> > rv;
	for (auto p_and_counts : required_half_rows) {
		TriParity parity = p_and_counts.first;
		std::map<Side_STP, size_t>& global_counts = rv[parity];
		std::map<Side_Decoder_STP, size_t> const& counts = p_and_counts.second;
		for (auto item : counts) {
			global_counts[to_Side_STP(item.first)] += item.second;
		}
	}
	return rv;
}

std::map<Side_STP, size_t> SynapseDriverRequirements::count_rows_per_side(
	std::map<TriParity, std::map<Side_STP, size_t> > const& required_half_rows_per_side,
	std::map<Side_STP, std::map<Parity, size_t> >& half_rows_assigned_to_parity)
{
	// we create a map with reversed keys ...
	// TODO: this might be done in a preceding function.
	std::map<Side_STP, std::map<TriParity, size_t> > map_reversed;

	for (auto p_and_counts : required_half_rows_per_side) {
		TriParity parity = p_and_counts.first;
		std::map<Side_STP, size_t> const& counts = p_and_counts.second;
		for (auto item : counts) {
			map_reversed[item.first][parity] = item.second;
		}
	}

	std::map<Side_STP, size_t> rv;

	for (auto item : map_reversed) {
		auto counts = item.second; // copy to remove constness attribute

		std::map<Parity, size_t>& assigned_half_row_counts =
			half_rows_assigned_to_parity[item.first];

		size_t count_per_side = std::max(counts[TriParity::even], counts[TriParity::odd]);

		assigned_half_row_counts[Parity::even] = counts[TriParity::even];
		assigned_half_row_counts[Parity::odd] = counts[TriParity::odd];
		if (counts[TriParity::any] > 0) {
			size_t abs_diff_even_odd =
				std::abs((int)counts[TriParity::even] - (int)counts[TriParity::odd]);
			if (counts[TriParity::any] > abs_diff_even_odd) {
				count_per_side +=
					std::max(0, int(std::ceil(0.5 * (counts[TriParity::any] - abs_diff_even_odd))));
			}

			if (abs_diff_even_odd > counts[TriParity::any]) {
				if (counts[TriParity::even] > counts[TriParity::odd]) {
					assigned_half_row_counts[Parity::odd] += counts[TriParity::any];
				} else {
					assigned_half_row_counts[Parity::even] += counts[TriParity::any];
				}
			} else { // difference between left and right is not sufficient to realize all
				size_t additional_half_rows = counts[TriParity::any] - abs_diff_even_odd;
				if (counts[TriParity::even] > counts[TriParity::odd]) {
					assigned_half_row_counts[Parity::odd] += abs_diff_even_odd;
				} else {
					assigned_half_row_counts[Parity::even] += abs_diff_even_odd;
				}
				// put more on the left than on the right
				assigned_half_row_counts[Parity::even] +=
					size_t(std::ceil(additional_half_rows / 2.));
				assigned_half_row_counts[Parity::odd] +=
					size_t(std::floor(additional_half_rows / 2.));
			}
		}
		rv[item.first] = count_per_side;
	}
	return rv;
}

std::map<STPMode, size_t> SynapseDriverRequirements::count_rows_per_STP(
	std::map<Side_STP, size_t> const& required_rows_per_side)
{
	std::map<STPMode, size_t> rv;
	for (auto item : required_rows_per_side) {
		rv[std::get<1>(item.first)] += item.second;
	}
	return rv;
}


std::map<STPMode, size_t> SynapseDriverRequirements::count_drivers_per_STP(
	std::map<STPMode, size_t> const& required_rows_per_stp)
{
	std::map<STPMode, size_t> rv;
	for (auto item : required_rows_per_stp) {
		rv[item.first] = size_t(std::ceil(0.5 * item.second));
	}
	return rv;
}

size_t
SynapseDriverRequirements::count_drivers(std::map<STPMode, size_t> const& required_rows_per_stp)
{
	size_t rv = 0;
	for (auto item : required_rows_per_stp) {
		rv += item.second;
	}
	return rv;
}


SynapseDriverRequirements::NeuronWidth SynapseDriverRequirements::extract_neuron_width(
	marocco::placement::NeuronBlockMapping const& neuron_block_mapping)
{
	NeuronWidth rv;

	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		placement::OnNeuronBlock const& onb = neuron_block_mapping[nb];

		for (auto it = onb.begin(); it != onb.end(); ++it) {
			size_t const hw_neuron_size = (*it)->neuron_size();
			size_t const hw_neuron_width = (*it)->neuron_width();

			for (auto& neuron : chunked(onb.neurons(it), hw_neuron_size)) {
				auto nrn = *neuron.begin();
				rv[nrn.toNeuronOnHICANN(nb)] = hw_neuron_width;
			}
		}
	}
	return rv;
}

std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
				   SynapseDriverRequirements::Side_Parity_count_per_synapse_type>
SynapseDriverRequirements::calc_target_synapses_per_synaptic_input_granularity(
	NeuronWidth const& neuron_width, SynapseTargetMapping const& syn_tgt_mapping)
{

	std::unordered_map<HMF::Coordinate::NeuronOnHICANN, Side_Parity_count_per_synapse_type> rv;

	for (auto const& item : neuron_width) {
		auto const& address = item.first;
		const size_t width = item.second;
		std::map<SynapseType, std::map<Side_Parity, size_t> >& targets_per_input = rv[address];

		for (size_t xx = address.x(); xx < address.x() + width; ++xx) {
			NeuronOnHICANN nrn_addr(X(xx), Y(address.y()));
			const Parity p = static_cast<Parity>(xx % 2);
			auto const& inputs_on_neuron = syn_tgt_mapping[nrn_addr];
			for (auto side : iter_all<SideHorizontal>()) {
				SynapseType syn_type = inputs_on_neuron[side];
				if (syn_type != SynapseType::None) {
					targets_per_input[syn_type][Side_Parity(side, p)]++;
				}
			}
		}
	}
	return rv;
}

std::unordered_map<NeuronOnHICANN,
				   std::map<SynapseType, SynapseDriverRequirements::SynapseColumnsMap> >
SynapseDriverRequirements::calc_synapse_type_to_synapse_columns_map(
	NeuronWidth const& neuron_width, SynapseTargetMapping const& syn_tgt_mapping)
{
	std::unordered_map<NeuronOnHICANN, std::map<SynapseType, SynapseColumnsMap> > rv;

	for (auto const& item : neuron_width) {
		auto const& address = item.first;
		const size_t width = item.second;
		std::map<SynapseType,
				 std::map<Side_Parity, std::vector<HMF::Coordinate::SynapseColumnOnHICANN> > >&
			synapse_columns_map_per_syntype = rv[address];

		for (size_t xx = address.x(); xx < address.x() + width; ++xx) {
			NeuronOnHICANN nrn_addr(X(xx), Y(address.y()));
			const Parity p = static_cast<Parity>(xx % 2);
			auto const& inputs_on_neuron = syn_tgt_mapping[nrn_addr];
			for (auto side : iter_all<SideHorizontal>()) {
				SynapseType syn_type = inputs_on_neuron[side];
				if (syn_type != SynapseType::None) {
					synapse_columns_map_per_syntype[syn_type][Side_Parity(side, p)].push_back(
						SynapseColumnOnHICANN(nrn_addr.x()));
				}
			}
		}
	}
	return rv;
}

std::unordered_map<NeuronOnHICANN,
				   std::map<SynapseType, SynapseDriverRequirements::SynapseColumnsMap> >
SynapseDriverRequirements::get_synapse_type_to_synapse_columns_map() const
{
	return calc_synapse_type_to_synapse_columns_map(mNeuronWidth, mSynapseTargetMapping);
}

std::pair<size_t, size_t> SynapseDriverRequirements::calc(
	std::vector<HardwareProjection> const& projections, graph_t const& graph) const
{
	std::map<Side_Parity_Decoder_STP, size_t> synapse_histogram;
	std::map<Side_Parity_Decoder_STP, size_t> synrow_histogram;
	return calc(projections, graph, synapse_histogram, synrow_histogram);
}

std::pair<size_t, size_t> SynapseDriverRequirements::_calc(
	SynapseCounts const& syn_counts,
	std::unordered_map<HMF::Coordinate::NeuronOnHICANN, Side_Parity_count_per_synapse_type> const&
		target_synapses_per_parity_and_synaptic_input,
	std::map<Side_Parity_Decoder_STP, size_t>& synapse_histogram,
	std::map<Side_Parity_Decoder_STP, size_t>& synrow_histogram)
{
	std::vector<std::map<Type_Decoder_STP, std::map<Side_Parity, size_t> > >
		half_rows_per_input_granularity;

	half_rows_per_input_granularity.reserve(syn_counts.get_counts().size());

	for (auto const& item : syn_counts.get_counts()) {
		NeuronOnHICANN const& nrn_addr = item.first;
		half_rows_per_input_granularity.push_back(
			count_half_rows_per_input_granularity(
				item.second, target_synapses_per_parity_and_synaptic_input.at(nrn_addr)));
	}

	std::vector<std::map<TriParity, std::map<Side_Decoder_STP, size_t> > > half_rows_per_triparity;
	std::vector<std::map<Type_Decoder_STP, Side_TriParity> > bio_to_hw_assignment;

	half_rows_per_triparity.reserve(half_rows_per_input_granularity.size());
	bio_to_hw_assignment.reserve(half_rows_per_input_granularity.size());

	for (auto const& item : half_rows_per_input_granularity) {
		bio_to_hw_assignment.push_back(std::map<Type_Decoder_STP, Side_TriParity>());
		half_rows_per_triparity.push_back(count_half_rows(item, bio_to_hw_assignment.back()));
	}

	std::map<TriParity, std::map<Side_Decoder_STP, size_t> > half_rows_per_triparity_global =
		count_half_rows_vec(half_rows_per_triparity);

	std::map<Side_STP, std::map<Parity, size_t> > half_rows_assigned_to_parity;
	std::map<Side_STP, size_t> rows_per_side = count_rows_per_side(
		count_half_rows_per_side(half_rows_per_triparity_global), half_rows_assigned_to_parity);

	size_t n_drivers = count_drivers(count_drivers_per_STP(count_rows_per_STP(rows_per_side)));


	// ok, now count the number of synapses and half synapse rows per HW Property
	std::map<Side_Parity_Decoder_STP, size_t> syn_counts_per_hw_property_global;
	std::map<Side_Parity_Decoder_STP, size_t> half_rows_assigned_per_parity_global;

	size_t ii = 0;
	for (auto const& item : syn_counts.get_counts()) {
		NeuronOnHICANN const& nrn_addr = item.first;
		auto const& bio_property_counts = item.second;

		std::map<Side_Parity_Decoder_STP, size_t> half_rows_per_hardware_property;
		std::map<Type_Decoder_STP, std::vector<Side_Parity> > assigned_side_parity; // not used
																					// further

		std::map<Side_Parity_Decoder_STP, size_t> syn_counts_per_hw_property =
			count_synapses_per_hardware_property(
				bio_property_counts, rows_per_side, bio_to_hw_assignment[ii],
				target_synapses_per_parity_and_synaptic_input.at(nrn_addr),
				half_rows_per_hardware_property, assigned_side_parity);

		for (auto const& item : syn_counts_per_hw_property) {
			syn_counts_per_hw_property_global[item.first] += item.second;
		}

		for (auto const& item : half_rows_per_hardware_property) {
			size_t& half_row_count = half_rows_assigned_per_parity_global[item.first];
			if (item.second > half_row_count)
				half_row_count = item.second;
		}
		ii++;
	}

	// count synapses
	size_t syn_count = 0;
	for (auto const& nrn_map : syn_counts.get_counts()) {
		for (auto const& item : nrn_map.second) {
			syn_count += item.second;
		}
	}
	// check synapse count again:
	{
		size_t syn_count2 = 0;
		for (auto const& item : syn_counts_per_hw_property_global) {
			syn_count2 += item.second;
		}
		if (syn_count != syn_count2)
			throw std::runtime_error(
				"there was an error when counting synapses per hw synapse property");
	}
	synapse_histogram = syn_counts_per_hw_property_global;
	synrow_histogram = half_rows_assigned_per_parity_global;
	return {n_drivers, syn_count};
}

std::pair<size_t, size_t> SynapseDriverRequirements::calc(
	std::vector<HardwareProjection> const& projections,
	graph_t const& graph,
	std::map<Side_Parity_Decoder_STP, size_t>& synapse_histogram,
	std::map<Side_Parity_Decoder_STP, size_t>& synrow_histogram) const
{
	SynapseCounts sc;

	auto const& revmap = mNeuronPlacement.placement();

	for (auto const& proj : projections)
	{
		graph_t::edge_descriptor pynn_proj = proj.projection();

		graph_t::vertex_descriptor target = boost::target(pynn_proj, graph);

		assignment::AddressMapping const& am = proj.source();
		std::vector<L1Address> const& addresses = am.addresses();

		assignment::PopulationSlice const& src_bio_assign = am.bio();

		size_t const src_bio_size   = src_bio_assign.size();
		size_t const src_bio_offset = src_bio_assign.offset();

		// now there is everything from the source, now need more info about target
		// population placement.

		ProjectionView const proj_view = graph[pynn_proj];

		size_t const src_neuron_offset_in_proj_view =
			getPopulationViewOffset(src_bio_offset, proj_view.pre().mask());

		// FIXME: some of the following seems to have been copied
		// verbatim from SynapseRouting.cpp
		std::vector<assignment::NeuronBlockSlice> const& hwassigns = revmap.at(target);
		for (auto const& hwassign: hwassigns)
		{
			auto const& terminal = hwassign.coordinate();
			if (terminal.toHICANNGlobal() == hicann())
			{
				placement::OnNeuronBlock const& onb = mNeuronPlacement.at(
				    terminal.toHICANNGlobal())[terminal.toNeuronBlockOnHICANN()];

				auto const it = onb.get(hwassign.offset());
				assert(it != onb.end());

				std::shared_ptr<placement::NeuronPlacement> const& trg_assign = *it;
				assignment::PopulationSlice const& trg_bio_assign = trg_assign->population_slice();
				size_t const trg_bio_size   = trg_bio_assign.size();
				size_t const trg_bio_offset = trg_bio_assign.offset();

				{
					// FIXME: Confirm and remove this:
					NeuronOnNeuronBlock first = *onb.neurons(it).begin();
					assert(first == hwassign.offset());
				}

				NeuronOnNeuronBlock const& first = hwassign.offset();

				size_t const hw_neuron_width = trg_assign->neuron_width();

				auto bio_weights = proj_view.getWeights(); // this is just a view, no copying

				SynapseType syntype_proj = toSynapseType(proj_view.projection()->target());

				auto dynamics = proj_view.projection()->dynamics();
				STPMode stp_proj = toSTPMode(dynamics);

				size_t const trg_neuron_offset_in_proj_view =
					getPopulationViewOffset(trg_bio_offset, proj_view.post().mask());

				size_t cnt=0;
				size_t src_neuron_in_proj_view = src_neuron_offset_in_proj_view;
				for (size_t src_neuron=src_bio_offset; src_neuron<src_bio_offset+src_bio_size; ++src_neuron)
				{
					if (!proj_view.pre().mask()[src_neuron]) {
						continue;
					}

					L1Address const& l1_address = addresses[cnt++];

					//boost::numeric::ublas::matrix_row<Connector::const_matrix_view_type> row(weights, src_neuron_in_proj_view);
					//auto weight_iterator = row.begin()+trg_neuron_offset_in_proj_view;

					size_t trg_neuron_in_proj_view = trg_neuron_offset_in_proj_view;
					for (size_t trg_neuron=trg_bio_offset; trg_neuron<trg_bio_offset+trg_bio_size; ++trg_neuron)
					{
						if (!proj_view.post().mask()[trg_neuron]) {
							continue;
						}

						double const weight = bio_weights(src_neuron_in_proj_view, trg_neuron_in_proj_view);
						//double const weight = *(weight_iterator++);
						if (!std::isnan(weight) && weight > 0.)
						{
							// FIXME: use coordinate conversion functions...
							size_t const trg_hw_offset =
							    terminal.toNeuronBlockOnHICANN().value() * 32 +
							    first.x() +
							    (trg_neuron - trg_bio_offset) * hw_neuron_width;
							sc.add(
								NeuronOnHICANN(X(trg_hw_offset), Y(0)), l1_address, syntype_proj,
								stp_proj);
						}
						++trg_neuron_in_proj_view;
					}

					++src_neuron_in_proj_view;
				}
			}
		} // all hw assignments
	} // all hw projections

	return _calc(
		sc, mTargetSynapsesPerSynapticInputGranularity, synapse_histogram, synrow_histogram);
}

std::map<Side_Parity_Decoder_STP, size_t>
SynapseDriverRequirements::count_synapses_per_hardware_property(
	// in
	std::map<Type_Decoder_STP, size_t> bio_property_counts,					// neuron-wise
	std::map<Side_STP, size_t> const& assigned_rows_per_side_stp,			// global
	std::map<Type_Decoder_STP, Side_TriParity> const& bio_to_hw_assignment, // neuron-wise
	std::map<SynapseType, std::map<Side_Parity, size_t> >
		target_synapses_per_parity_and_synaptic_input, // neuron-wise
	// out
	std::map<Side_Parity_Decoder_STP, size_t>& half_rows_per_hardware_property, // neuron-wise
	std::map<Type_Decoder_STP, std::vector<Side_Parity> >& assigned_side_parity // neuron-wise
	)
{
	std::map<Side_Parity_Decoder_STP, size_t> syn_count;

	std::map<Type_Decoder_STP, std::pair<Side, size_t> > postponed_any_requests;
	std::map<Side_Parity_STP, size_t> used_half_rows;

	// first process synapses that are bound to even or odd columns
	for (auto const& item : bio_property_counts) {
		Type_Decoder_STP const& bio_prop = item.first;
		size_t const synapse_count = item.second;

		Side side;
		TriParity triparity;
		std::tie(side, triparity) = bio_to_hw_assignment.at(bio_prop);

		if (triparity != TriParity::any) {
			// get num required half rows.
			SynapseType syntype;
			DriverDecoder decoder;
			STPMode stp;
			std::tie(syntype, decoder, stp) = bio_prop;
			Parity parity = static_cast<Parity>(triparity);
			Side_Parity const side_parity(side, parity);
			size_t const syns_per_half_row =
				target_synapses_per_parity_and_synaptic_input.at(syntype).at(side_parity);
			size_t const req_half_rows = std::ceil(double(synapse_count) / syns_per_half_row);
			// store syn count
			syn_count[Side_Parity_Decoder_STP(side, parity, decoder, stp)] += synapse_count;
			// store the assignment
			assigned_side_parity[bio_prop] = std::vector<Side_Parity>(req_half_rows, side_parity);
			// store used half rows
			used_half_rows[Side_Parity_STP(side, parity, stp)] += req_half_rows;
			// store required half rows
			half_rows_per_hardware_property[Side_Parity_Decoder_STP(side, parity, decoder, stp)] +=
				req_half_rows;
		} else {
			postponed_any_requests[bio_prop] = std::make_pair(side, synapse_count);
		}
	}
	// now process synapses that can be realized in either column (TriParity::any)
	for (auto const& item : postponed_any_requests) {
		Type_Decoder_STP const& bio_prop = item.first;
		size_t const synapse_count = item.second.second;
		Side side = item.second.first;

		// get num required half rows.
		SynapseType syntype;
		DriverDecoder decoder;
		STPMode stp;
		std::tie(syntype, decoder, stp) = bio_prop;

		size_t const syns_per_half_row = target_synapses_per_parity_and_synaptic_input.at(syntype)
											 .at(Side_Parity(side, Parity::even));
		size_t req_half_rows = std::ceil(double(synapse_count) / syns_per_half_row);

		// number of requested half rows should be equal for both parities.
		// (number of synapses per half rows may differ, e.g. when there is only 1 synapse)
		size_t const syns_per_half_row_odd =
			target_synapses_per_parity_and_synaptic_input.at(syntype)
				.at(Side_Parity(side, Parity::odd));
		size_t const req_half_rows_odd = std::ceil(double(synapse_count) / syns_per_half_row_odd);
		assert(req_half_rows == req_half_rows_odd);

		Side_Parity_STP const c_even(side, Parity::even, stp);
		Side_Parity_STP const c_odd(side, Parity::odd, stp);
		size_t req_synapses = synapse_count;

		while (req_half_rows > 0) {
			Side_Parity_STP choice;
			// use the parity with the lower number of used counts
			if (used_half_rows[c_even] <= used_half_rows[c_odd]) {
				choice = c_even;
			} else {
				choice = c_odd;
			}
			// store syn count
			size_t const syns_for_this_half_row = std::min(req_synapses, syns_per_half_row);
			Parity parity = std::get<1>(choice);
			syn_count[Side_Parity_Decoder_STP(side, parity, decoder, stp)] +=
				syns_for_this_half_row;
			half_rows_per_hardware_property[Side_Parity_Decoder_STP(side, parity, decoder, stp)]++;
			// store the assignment
			assigned_side_parity[bio_prop].push_back(Side_Parity(side, parity));
			// store used half rows
			used_half_rows[choice]++;
			// decrease requirements
			req_half_rows--;
			req_synapses -= syns_for_this_half_row;
		}
		assert(req_synapses == 0);
	}
	// check that used rows are lower equal than assigned rows;
	for (auto const& item : used_half_rows) {
		Side_Parity_STP const& key_used = item.first;
		Side side;
		STPMode stp;
		std::tie(side, std::ignore, stp) = key_used;
		if (!(item.second <= assigned_rows_per_side_stp.at(Side_STP(side, stp))))
			throw std::runtime_error("more half rows assigned per Side_STP than allowed!");
	}

	return syn_count;
}

template<typename Archiver>
void SynapseCounts::serialize(Archiver& ar, const unsigned int)
{
	using namespace boost::serialization;
	ar & make_nvp("counts", mCounts);
}

} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::SynapseCounts)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::SynapseCounts)
