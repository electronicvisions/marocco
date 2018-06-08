#include "marocco/routing/SynapseDriverRequirements.h"

#include <array>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>
#include <boost/assert.hpp>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/routing/util.h"

// One LocalRoute unwrapped
// ========================
//size_t num_sources = local_route.numSources();
//Route::BusSegment targetBus = local_route.targetBus();
//Route const& route = local_route.route();

//std::vector<HardwareProjection> const& projections = route.projections();

//for (auto const& proj : projections)
//{
	//graph_t::edge_descriptor pynn_proj = proj.projection();

	//assignment::AddressMapping const& am = proj.source();
	//std::vector<L1Address> const& addresses = am.addresses();
	//assignment::PopulationSlice const& bio = am.bio();

	//graph_t::vertex_descriptor pop = bio.population();
	//size_t size   = bio.size();
	//size_t offset = bio.offset();
//}

using namespace HMF::Coordinate;
using namespace HMF::HICANN;

namespace marocco {
namespace routing {

namespace {

struct LessByValue
{
	template <typename T, typename U>
	bool operator()(T const& lhs, U const& rhs) const
	{
		return lhs.second < rhs.second;
	}
};

/// SynapseHistogram only used for `SynapseDriverRequirements` not necessary for
/// public interface.
/// Holds histograms for each combination of 2MSB pattern and synaptic input target 
/// for hardware neurons on a HICANN.
///
/// @note: although this class stores counts for single hardware neurons, currently only 
///        the top left neuron of the compound neurons are used 
class SynapseHistogram
{
public:
	typedef SynapseDriverRequirements::Histogram Histogram;

	/// increase synapse histogram for target neuron 'p', for DriverDecoderMask 
	/// of source 'addr' and target 'type'
	void add(
		HMF::Coordinate::NeuronOnHICANN const& p,
		HMF::HICANN::L1Address const& addr,
		Projection::synapse_type const& type)
	{
		mHistogram[p][array_index(addr.getDriverDecoderMask(), type)] += 1;
	}

	/// get the number of synapses per target neuron required from sources 
	/// with the same DriverDecoder as the given addr, and the given synapse type.
	size_t get(
		HMF::Coordinate::NeuronOnHICANN const& p,
		HMF::HICANN::L1Address const& addr,
		Projection::synapse_type const& type) const
	{
		return mHistogram.at(p).at(array_index(addr.getDriverDecoderMask(), type));
	}

	/** returns the number of required synapse drivers and the total number of synapses
	 *  for the previously collected synapse histogram.
	 *
	 *  @param nbm neuron block mapping
	 *  @param synrow_histogram
	 *         for each combination of 2MSB pattern and synaptic input target 
	 *         (exc/inh) the number of half synapse row required to realize
	 *         all synapses
	 */
	std::pair<size_t, size_t>
	num_syndrv(placement::NeuronBlockMapping const& nbm, Histogram& synrow_histogram) const;

	/// returns the histogram of total synapses for each combination of 2MSB and synaptic target
	Histogram get_histogram() const;

private:
	/// returns the array index for a pair of (DriverDecoder, synapse_type).
	static size_t array_index(
		HMF::HICANN::DriverDecoder const& addr,
		Projection::synapse_type const& type)
	{
		if (type == "excitatory") {
			return addr;
		} else if (type == "inhibitory") {
			return HMF::HICANN::DriverDecoder::end+addr;
		} else {
			throw std::runtime_error("unknown synapse target type");
		}
	}

	typedef std::unordered_map<
			HMF::Coordinate::NeuronOnHICANN,
			Histogram
		> mapped_type;

	mapped_type mHistogram; /// map from hardware neuron coordinate to histograms
};



std::pair<size_t, size_t>
SynapseHistogram::num_syndrv(placement::NeuronBlockMapping const& nbm,
							 Histogram& synrow_histogram) const
{
	// nr of half synapse rows must be counted individually for excitatory and inhibitory target
	std::array<size_t, 2> num_half_syn_rows_per_type{{0}}; // 0: excitatory, 1: inhibitory
	size_t num_syns = 0;

	for (size_t msb=0; msb<std::tuple_size<mapped_type::mapped_type>::value; ++msb)
	{
		// a map to store the number of needed HalfSynapseRows per target neuron.
		typedef std::set<size_t> needed_t;
		needed_t needed;

		// first compute SynapseDriver needs for every target neuron for this route.
		for (auto const& entry : mHistogram)
		{
			auto const nrn = entry.first.toNeuronOnNeuronBlock();
			auto const block = entry.first.toNeuronBlockOnHICANN();

			placement::OnNeuronBlock const& onb = nbm.at(block);
			auto const it = onb.get(nrn);

			BOOST_ASSERT_MSG(it != onb.end(),
			                 "target neuron unassigned in OnNeuronBlock");

			//assignment::PopulationSlice const bio = (*it)->population_slice();
			size_t const width = (*it)->neuron_width();

			// we need to devide the number of needed synapses by half the geometric
			// width of the target neuron to get the number of needed half synapse rows.
			// Wider neurons, can realize more synapes on a single horizontal row.
			double const num_synapses = entry.second[msb];
			num_syns += num_synapses;
			size_t const num_half_synapse_rows = std::ceil(num_synapses/double(width)*2.);
			needed.insert(num_half_synapse_rows);
		}


		// The number of needed SynapseDrivers to realize ALL synapses is the maximum
		// needed over all target neurons.
		if (!needed.empty()) {
			size_t const local_half_syn_rows = *needed.rbegin();
			num_half_syn_rows_per_type[ msb/HMF::HICANN::DriverDecoder::end ] += local_half_syn_rows;
			synrow_histogram[msb] = local_half_syn_rows;
		} else {
			synrow_histogram[msb] = 0;
		}
	}
	// Furthermore, to get from the number of
	// needed half synapse rows to the number of needed SynapseDrivers we need to
	// first count the number of synapse rows per input target (exc,inh), i.e. divide the number of 
	// half synapse rows by 2 and round to the ceiling;
	// The same then applies to the number of drivers, which is the number of rows divided by two 
	// and rounded to the ceiling
	//
	// NOTE: this function assumes that each all neurons have at least a neuron size of 4.
	// Otherwise, we could not divide by 2 to get the number of synapse rows per target (exc/inh)
	// from the number of half synapse rows. (cf. #1565)
	size_t const num_exc_syn_rows = std::ceil(num_half_syn_rows_per_type[0]/2.);
	size_t const num_inh_syn_rows = std::ceil(num_half_syn_rows_per_type[1]/2.);
	size_t const num_drivers = std::ceil( (num_exc_syn_rows + num_inh_syn_rows)/2. );

	return std::pair<size_t, size_t>(num_drivers, num_syns);
}



SynapseHistogram::Histogram SynapseHistogram::get_histogram() const
{
	Histogram res = {{0,0,0,0,0,0,0,0}};

	for (auto const& trgneuron : mHistogram)
	{
		for (size_t ii=0; ii<res.size(); ++ii) {
			res[ii] += trgneuron.second[ii];
		}
	}

	return res;
}

} // anonymous

SynapseDriverRequirements::SynapseDriverRequirements(HICANNGlobal const& hicann,
									   placement::NeuronPlacementResult const& nrnpl) :
	mHICANN(hicann),
	mNeuronPlacement(nrnpl)
{}

std::pair<size_t, size_t>
SynapseDriverRequirements::calc(
	std::vector<HardwareProjection> const& projections,
	graph_t const& graph,
	Histogram& synapse_histogram,
	Histogram& synrow_histogram)
{
	SynapseHistogram histogram;

	auto const& revmap = mNeuronPlacement.placement();
	auto const projmap = boost::get(projection_t(), graph);

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

		ProjectionView const proj_view = boost::get(projmap, pynn_proj);

		size_t const src_neuron_offset_in_proj_view =
			getPopulationViewOffset(src_bio_offset, proj_view.pre().mask());

		// STHALPEITSCHE, NO REFERENCE!1!!
		assignment::Mapping const target_hw_assignments = revmap.get(target);

		// FIXME: some of the following seems to have been copied
		// verbatim from SynapseRouting.cpp
		std::vector<assignment::Hardware> const& hwassigns = target_hw_assignments.assignment();
		for (auto const& hwassign: hwassigns)
		{
			auto const& terminal = hwassign.get();
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
							histogram.add(NeuronOnHICANN(X(trg_hw_offset), Y(0)),
							              l1_address, proj_view.projection()->target());
						}
						++trg_neuron_in_proj_view;
					}

					++src_neuron_in_proj_view;
				}
			}
		} // all hw assignments
	} // all hw projections

	synapse_histogram = histogram.get_histogram();

	return histogram.num_syndrv(mNeuronPlacement.at(hicann()), synrow_histogram);
}

std::pair<size_t, size_t>
SynapseDriverRequirements::calc(
	std::vector<HardwareProjection> const& projections,
	graph_t const& graph)
{
	Histogram hist0, hist1;
	return calc(projections, graph, hist0, hist1);
}

} // namespace routing
} // namespace marocco
