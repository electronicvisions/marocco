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
	marocco::placement::Result const& placement_result,
	SynapseTargetMapping const& syn_tgt_mapping)
	: mHICANN(hicann),
	  mPlacementResult(placement_result),
	  mSynapseTargetMapping(syn_tgt_mapping),
	  mNeuronWidth(extract_neuron_width(placement_result.internal.denmem_assignment, mHICANN)),
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

std::map<Side_STP, size_t>
SynapseDriverRequirements::count_rows_per_side(
    std::map<Side_Parity_STP, size_t> const& required_half_rows_per_parity)
{
	std::map<Side_STP, size_t> rv;
	for (auto const& item : required_half_rows_per_parity) {
		Side side;
		STPMode stp;
		std::tie(side, std::ignore, stp) = item.first;
		Side_STP key(side, stp);
		rv[key] = std::max(rv[key], item.second); // max of all parities
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

std::map<Side_Parity_STP, size_t>
SynapseDriverRequirements::resolve_triparity(
    std::map<TriParity, std::map<Side_Decoder_STP, size_t> > const& half_rows_per_triparity,
    std::map<Side_Parity_Decoder_STP, size_t>& synrow_hist,
    std::map<Side_Decoder_STP, std::vector<Parity> >& assignment_to_parity)
{
	synrow_hist.clear();
	assignment_to_parity.clear();

	std::map<Side_Parity_STP, size_t> used_half_rows; // return value

	// first process  TriParity even and odd
	for (auto const& item : half_rows_per_triparity) {
		TriParity triparity = item.first;

		if (triparity == TriParity::any) {
			continue;
		}

		Parity parity = static_cast<Parity>(triparity);
		for (auto const& item2 : item.second) {
			Side side;
			DriverDecoder decoder;
			STPMode stp;
			std::tie(side, decoder, stp) = item2.first;

			used_half_rows[Side_Parity_STP(side, Parity(triparity), stp)] += item2.second;

			synrow_hist[Side_Parity_Decoder_STP(side, parity, decoder, stp)] += item2.second;
		}
	}

	// finally process triparity::any
	auto it_any = half_rows_per_triparity.find(TriParity::any);
	if (it_any != half_rows_per_triparity.end()) {
		for (auto const& item : it_any->second) {
			Side side;
			DriverDecoder decoder;
			STPMode stp;
			std::tie(side, decoder, stp) = item.first;
			// consider the number of rows per side!!
			// same procedure as in count_synapses_per_hardware_property
			// what is better: all to the same parity, or round-robin?
			size_t req_half_rows = item.second;

			Side_Parity_STP const c_even(side, Parity::even, stp);
			Side_Parity_STP const c_odd(side, Parity::odd, stp);

			while (req_half_rows > 0) {
				Side_Parity_STP choice;
				// use the parity with the lower number of used counts
				if (used_half_rows[c_even] <= used_half_rows[c_odd]) {
					choice = c_even;
				} else {
					choice = c_odd;
				}
				Parity parity = std::get<1>(choice);
				synrow_hist[Side_Parity_Decoder_STP(side, parity, decoder, stp)]++;
				// store the assignment
				assignment_to_parity[Side_Decoder_STP(side, decoder, stp)].push_back(parity);
				// store used half rows
				used_half_rows[choice]++;
				// decrease requirements
				req_half_rows--;
			}
		}
	}

	return used_half_rows;
}


SynapseDriverRequirements::NeuronWidth SynapseDriverRequirements::extract_neuron_width(
	placement::internal::Result::denmem_assignment_type const& denmem_assignment,
	HICANNOnWafer const& hicann)
{
	NeuronWidth rv;
	auto it = denmem_assignment.find(hicann);
	if (it == denmem_assignment.end()) {
		return rv;
	}

	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		// TODO: Use .find(NeuronBlockOnWafer()) -> LogicalNeuron() interface
		placement::internal::OnNeuronBlock const& onb = (it->second)[nb];

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

	// resolve triparity to parity and calculate synrow histogram
	std::map<Side_Decoder_STP, std::vector<Parity> > triparity_assignment_to_parity;
	std::map<Side_Parity_STP, size_t> half_rows_per_parity = resolve_triparity(
	    half_rows_per_triparity_global, synrow_histogram, triparity_assignment_to_parity);

	// total number of required drivers
	size_t n_drivers = count_drivers(
	    count_drivers_per_STP(count_rows_per_STP(count_rows_per_side(half_rows_per_parity))));

	// count the number of synapses per hardware property
	size_t ii = 0;
	for (auto const& item : syn_counts.get_counts()) {
		NeuronOnHICANN const& nrn_addr = item.first;
		auto const& bio_property_counts = item.second;

		std::map<Side_Parity_Decoder_STP, size_t> syn_counts_per_hw_property =
		    count_synapses_per_hardware_property(
		        bio_property_counts, bio_to_hw_assignment[ii],
		        target_synapses_per_parity_and_synaptic_input.at(nrn_addr),
		        triparity_assignment_to_parity, synrow_histogram);

		for (auto const& item : syn_counts_per_hw_property) {
			synapse_histogram[item.first] += item.second;
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
		for (auto const& item : synapse_histogram) {
			syn_count2 += item.second;
		}
		if (syn_count != syn_count2)
			throw std::runtime_error(
				"there was an error when counting synapses per hw synapse property");
	}

	return {n_drivers, syn_count};
}

std::pair<size_t, size_t> SynapseDriverRequirements::calc(
	std::vector<HardwareProjection> const& projections,
	graph_t const& graph,
	std::map<Side_Parity_Decoder_STP, size_t>& synapse_histogram,
	std::map<Side_Parity_Decoder_STP, size_t>& synrow_histogram) const
{
	SynapseCounts sc;

	auto const& revmap = mPlacementResult.internal.primary_denmems_for_population;

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
		for (auto const& primary_neuron : revmap.at(target)) {
			auto const terminal = primary_neuron.toNeuronBlockOnWafer();
			if (terminal.toHICANNOnWafer() == hicann().toHICANNOnWafer()) {
				// TODO: Use .find(vertex_descriptor) -> LogicalNeuron() interface
				placement::internal::OnNeuronBlock const& onb =
					mPlacementResult.internal.denmem_assignment.at(
						terminal.toHICANNOnWafer())[terminal.toNeuronBlockOnHICANN()];

				auto const it = onb.get(primary_neuron.toNeuronOnNeuronBlock());
				assert(it != onb.end());

				std::shared_ptr<placement::internal::NeuronPlacementRequest> const& trg_assign = *it;
				assignment::PopulationSlice const& trg_bio_assign = trg_assign->population_slice();
				size_t const trg_bio_size   = trg_bio_assign.size();
				size_t const trg_bio_offset = trg_bio_assign.offset();

				{
					// FIXME: Confirm and remove this:
					NeuronOnNeuronBlock first = *onb.neurons(it).begin();
					assert(first == primary_neuron.toNeuronOnNeuronBlock());
				}

				NeuronOnNeuronBlock const& first = primary_neuron.toNeuronOnNeuronBlock();

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
    std::map<Type_Decoder_STP, size_t> const& bio_property_counts,          // neuron-wise
    std::map<Type_Decoder_STP, Side_TriParity> const& bio_to_hw_assignment, // neuron-wise
    std::map<SynapseType, std::map<Side_Parity, size_t> > const&
        target_synapses_per_parity_and_synaptic_input,                               // neuron-wise
    std::map<Side_Decoder_STP, std::vector<Parity> > const& assignment_to_parity,    // global
    std::map<Side_Parity_Decoder_STP, size_t> const& half_rows_per_hardware_property // global for
                                                                                     // check
    )
{
	std::map<Side_Parity_Decoder_STP, size_t> syn_count; // return value
#ifndef MAROCCO_NDEBUG
	std::map<Side_Parity_Decoder_STP, size_t> used_half_rows; // for check
#endif // MAROCCO_NDEBUG

	// first process synapses that are bound to even or odd columns
	for (auto const& item : bio_property_counts) {
		Type_Decoder_STP const& bio_prop = item.first;
		size_t synapse_count = item.second;

		Side side;
		TriParity triparity;
		std::tie(side, triparity) = bio_to_hw_assignment.at(bio_prop);

		SynapseType syntype;
		DriverDecoder decoder;
		STPMode stp;
		std::tie(syntype, decoder, stp) = bio_prop;

		if (triparity != TriParity::any) {
			// get num required half rows.
			Parity parity = static_cast<Parity>(triparity);
			Side_Parity const side_parity(side, parity);
			size_t const syns_per_half_row =
				target_synapses_per_parity_and_synaptic_input.at(syntype).at(side_parity);
			size_t const req_half_rows = std::ceil(double(synapse_count) / syns_per_half_row);
			// store syn count
			syn_count[Side_Parity_Decoder_STP(side, parity, decoder, stp)] += synapse_count;
#ifndef MAROCCO_NDEBUG
			// store used half rows for consistency check
			used_half_rows[Side_Parity_Decoder_STP(side, parity, decoder, stp)] += req_half_rows;
#endif // MAROCCO_NDEBUG
		} else {
			// TriParity::any
			std::vector<Parity> const& parities =
			    assignment_to_parity.at(Side_Decoder_STP(side, decoder, stp));
			auto p_it = parities.cbegin();
			while (synapse_count > 0) {
				assert(p_it != parities.cend());
				Parity parity = *p_it;
				size_t const syns_per_half_row =
				    target_synapses_per_parity_and_synaptic_input.at(syntype).at(
				        Side_Parity(side, parity));
				size_t assigned_syns = std::min(synapse_count, syns_per_half_row);
				// store syn count
				syn_count[Side_Parity_Decoder_STP(side, parity, decoder, stp)] += assigned_syns;
#ifndef MAROCCO_NDEBUG
				// store used half rows for consistency check
				used_half_rows[Side_Parity_Decoder_STP(side, parity, decoder, stp)]++;
#endif // MAROCCO_NDEBUG
				synapse_count -= assigned_syns;
				p_it++;
			}
		}
	}

#ifndef MAROCCO_NDEBUG
	// consistency check
	for (auto const& item : used_half_rows) {
		if (!(item.second <= half_rows_per_hardware_property.at(item.first)))
			throw std::runtime_error("Too many half rows assigned per hardware synapse property");
	}
#endif // MAROCCO_NDEBUG

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
