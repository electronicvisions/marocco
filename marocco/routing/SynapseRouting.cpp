#include "marocco/routing/SynapseRouting.h"

#include <cstdlib>
#include <stdexcept>

#include "marocco/routing/SynapseDriverSA.h"
#include "marocco/routing/Fieres.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "marocco/routing/SynapseRowIterator.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/util.h"
#include "marocco/Logger.h"

#include "hal/Coordinate/iter_all.h"

#include "HMF/SynapseDecoderDisablingSynapse.h"

#include <boost/make_shared.hpp>

// NOTE: always use clear vertex and maybe edge lists, rather than vectors,
// because we have a rather dynamically changing graph.
// Resource allocate for e.g. synapse driver will remove edges.


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

/** reference to a synaptic weight between a certain bin (2MSB, syntype) and a target neuron.
 *
 * hw synapses are looked up as follows:
 *   SubRows subrows;
 *   WeighRef ref;
 *   synrow_on_hicann = subrows[ref.row].first;
 *   col_idx = tgt_neuron_offset + ref.col;
 */
struct WeightRef
{
	std::int32_t row =  0; /// idx of the half synapse row in the list of assigned half synapse rows (SynapseRowManager::SubRows).
	std::int32_t col = -1; /// idx of the column relative to the column offset of the target neuron.
};

/** The synapse row manager is responsible for distributing the half synapse rows of 
 * the assigned synapse drivers to the respective synapse types and 2MSB patterns.
 *
 * The SynapseRowManager is constructed with the result of the Driver Assignment algorithm
 * (Fieres or SynapseDriverSA).
 * Then, the information about originally required synrows and synapses per bin (syntype, 2MSB)
 * is added with the  init(...) method
 */
class SynapseRowManager
{
public:
	struct OutOfSynapseRows {};

	typedef std::unordered_map<
			HMF::Coordinate::VLineOnHICANN,
			std::vector<DriverAssignment>
		> Result;
	typedef std::vector<std::pair<
		HMF::Coordinate::SynapseRowOnHICANN, bool /*offset*/>> SubRows; // vector of half synapse rows
	typedef std::array<SubRows, 2 /*exc/inh*/ * 4 /*MSBs*/> Assignment;// assigned half synapse rows for each "bin"

	typedef SynapseDriverRequirements::Histogram Histogram;
	typedef std::unordered_map<VLineOnHICANN, Histogram> HistMap;

	/// constructs the synapse row manager with results from driver assignment algorithm
	SynapseRowManager(Result const& resources);

	/// for each input vline, distribute the half synapse rows of the assigned drivers
	/// to the different bins (syntype, 2MSB).
	/// If less half rows are assigned than needed, the same assignment reduction
	/// process as in the Fieres Algorithm for Synapse drivers is done.
	/// hence the assignment is fair across bins!
	///
	/// @warn seems to does not distinguish well between excitatory inhibitory rows.
	///       hence, it might happen that one inhibitory half synapse row is not realized.
	///       Note, however, that this constraint is neither considered for the synapse
	///       driver requirements!!
	void init(HistMap const& synapse_hist, HistMap const& synrow_hist);

	/// Returns the list of half synapse rows assigned to a vline for a given bin (type,2MSB)
	SubRows const& getRows(
		HMF::Coordinate::VLineOnHICANN const& vline,
		DriverDecoder const& addr,
		Projection::synapse_type const& type);

	Assignment const& get(HMF::Coordinate::VLineOnHICANN const& vline) const;

	/// returns the DriverDecoder (2MSB) and the synapse depending for a given index of the Histograms
	std::pair<HMF::HICANN::DriverDecoder, Projection::synapse_type>
	fromIndex(size_t idx) const;

	void check(size_t chain_length);

	/// calculates the array index in the histogram out of the 2MSB L1 address and the synapse type
	static size_t array_index(
		HMF::HICANN::DriverDecoder const& addr,
		Projection::synapse_type const& type);

	friend std::ostream& operator<< (std::ostream& os, SynapseRowManager const& mgr);

private:
	/// allocates a new mapping. Throw `OutOfSynapseRows` if no more rows are
	/// available.
	SubRows::value_type const& allocateNew(
		HMF::Coordinate::VLineOnHICANN const& vline,
		DriverDecoder const& addr,
		Projection::synapse_type const& type);

	/// tracks the current synapse row assignment for each local route and each
	/// combination of MSBs and exc/inh. For all this combinations we need /
	/// individual SynapseRows, because the necessary configuration is part of /
	/// that row and not the individual synapse.
	std::unordered_map<HMF::Coordinate::VLineOnHICANN /*route id*/, Assignment> mAllocation;

	/// mapping of vlines to iterators, pointing to the next free SynapseRow
	std::unordered_map<HMF::Coordinate::VLineOnHICANN,
		std::pair<SynapseRowIterator /*begin*/, SynapseRowIterator const /*end*/> > mNextFree;

	/// the number of drivers assigned to each vline
	std::unordered_map<HMF::Coordinate::VLineOnHICANN, size_t> mLines;

	/// stores for each synapse row the next offset to use for allocation of half synapse rows.
	std::unordered_map<HMF::Coordinate::SynapseRowOnHICANN, size_t> mRows;
};

struct Most
{
	template<typename T>
	bool operator () (T const& a, T const& b)
	{
		return a > b;
	}
};

SynapseRowManager::SynapseRowManager(Result const& resources) :
	mAllocation(), mNextFree(), mLines(), mRows()
{
	for (auto const& entry : resources)
	{
		VLineOnHICANN const& vline = entry.first;
		std::vector<DriverAssignment> const& vec = entry.second;

		auto  it = vec.begin();
		auto eit = vec.end();

		mNextFree.insert(std::make_pair(vline,
			std::make_pair(SynapseRowIterator( it, eit),
						   SynapseRowIterator(eit, eit))));

		mLines[vline] = 0;
		for (auto const& da : vec) {
			mLines[vline] += da.drivers.size();
		}
	}
}

void SynapseRowManager::init(HistMap const& synapse_hist, HistMap const& synrow_hist)
{
	for (auto const& entry : mLines)
	{
		VLineOnHICANN const& vline = entry.first;
		Histogram assigns {{}};
		size_t assigned=0;

		size_t const avail_half_syn_rows = entry.second * 4;

		size_t const req_half_syn_rows = std::accumulate(
			synrow_hist.at(vline).begin(), synrow_hist.at(vline).end(), 0);

		if (avail_half_syn_rows >= req_half_syn_rows)
		{
			// we got sufficient rows, simply assign, assign according to
			// requiresments
			auto const& hist = synrow_hist.at(vline);
			for (size_t ii=0; ii<assigns.size(); ++ii)
			{
				size_t const b = hist.at(ii);
				assigns[ii] = b;
				assigned+=b;
			}

			// immer schoen aufessen auch wenn man satt ist
			assert(assigned == req_half_syn_rows);
			//if (avail_half_syn_rows > req_half_syn_rows) {
				//std::multimap<size_t, size_t> sorted_syns;
				//auto const& synhist = synapse_hist.at(vline);
				//for (auto it=synhist.begin(); it!=synhist.end(); ++it) {
					//if (*it>0) {
						//size_t const pos = it-synhist.begin();
						//sorted_syns.insert(std::make_pair(*it, pos));
					//}
				//}

				//while (assigned<avail_half_syn_rows) {
					//for (auto const& v : sorted_syns)
					//{
						//size_t const pos = v.second;
						//assigns[pos]++;
						//assigned++;
						//if (assigned>=avail_half_syn_rows) {
							//break;
						//}
					//}
				//}
			//}
		} else {
			// we didn't get sufficient rows, now assign relative to number of
			// synapseses neurons for each MSB/TYPE
			auto const& hist = synapse_hist.at(vline);
			double const syns = std::accumulate(hist.begin(), hist.end(), 0);

			typedef std::multimap<double, Histogram::iterator, Most> Map;
			Map too_many;
			Map too_few;
			for (size_t ii=0; ii<hist.size(); ++ii)
			{
				if (hist[ii]==0) {
					assigns[ii] = 0;
					continue;
				}

				double const rel = double(hist[ii])/syns*double(avail_half_syn_rows);
				size_t const t = std::max<size_t>(1, rel);
				assigns[ii] = t;
				assigned += t;

				double const delta = double(t) - rel;
				if (delta>0.) {
					// too many
					too_many.insert(std::make_pair(delta, assigns.begin()+ii));
				} else {
					// too few
					too_few.insert(std::make_pair(std::abs(delta), assigns.begin()+ii));
				}
			}

			// too many assigned
			while (assigned>avail_half_syn_rows) {
				for (auto it = too_many.begin(); it!=too_many.end(); ++it)
				{
					size_t& val = *(it->second);
					if (val>0) {
						MAROCCO_DEBUG("rescale synapse rows -1: " << vline << " "
						  << std::distance(assigns.begin(), it->second));
						assigned--;
						val--;
					}

					if (assigned==avail_half_syn_rows) {
						break;
					}
				}
			}

			// too few assigned
			size_t cnt = 0;
			while (assigned<avail_half_syn_rows) {
				++cnt;
				for (auto it = too_few.begin(); it!=too_few.end(); ++it)
				{
					MAROCCO_DEBUG("rescale synapse rows +1: " << vline << " "
					  << std::distance(assigns.begin(), it->second));
					size_t& val = *(it->second);
					val++;
					assigned++;

					if (assigned==avail_half_syn_rows) {
						break;
					}
				}
			}
		}

		//if (assigned!=avail_half_syn_rows) {
			//throw std::runtime_error("error during synapse row assignment");
		//}

		// Now assign to real half synapse rows
		for (size_t ii=0; ii<assigns.size(); ++ii)
		{
			SynapseRowManager::SubRows::value_type row;
			// std::pair<HMF::HICANN::DriverDecoder, Projection::synapse_type>
			auto const keys = fromIndex(ii);
			for (size_t jj=0; jj<assigns[ii]; ++jj)
			{
				row = allocateNew(vline, keys.first, keys.second);
			}

			// respect input circuit granularity  (row must be either exc or inh)
			//
			// i.e. when all excitatory bins are assigned (i==3), and the last 
			// assigned half row has offset 0, we ristk to map both exc. and inh.
			// half rows to one synapse row.
			// hence, in case there are both types assigned, we can add another
			// half synpse row of excitatory type to offset 1.
			if (ii==3 && row.second==0) {
				bool flag1 = false; // there are half rows assigned for inhibitory synapses
				bool flag2 = false; // there are half rows assigned for excitatory synapses
				for (size_t jj=0; jj<assigns.size()/2; ++jj) {
					flag2 |= assigns[jj];
				}
				for (size_t jj=assigns.size()/2; jj<assigns.size(); ++jj) {
					flag1 |= assigns[jj];
				}
				if (flag1 && flag2) {
					row = allocateNew(vline, keys.first, keys.second);
				}
			}
		}
	} // for all synapse driver assignments on this side
}

SynapseRowManager::SubRows const& SynapseRowManager::getRows(
	HMF::Coordinate::VLineOnHICANN const& vline,
	DriverDecoder const& addr,
	Projection::synapse_type const& type)
{
	return mAllocation[vline][array_index(addr, type)];
}


/// allocates a new mapping. Throw `OutOfSynapseRows` if no more rows are
/// available.
SynapseRowManager::SubRows::value_type const&
SynapseRowManager::allocateNew(
	HMF::Coordinate::VLineOnHICANN const& vline,
	DriverDecoder const& addr,
	Projection::synapse_type const& type)
{
	size_t const index = array_index(addr, type);
	SubRows& list = mAllocation[vline][index];


	auto& it = mNextFree[vline];
	if (it.first == it.second) {
		throw OutOfSynapseRows();
	}

	// now, the checker board fun begins
	SynapseRowOnHICANN const row = *it.first;

	size_t& offset = mRows[row];
	if (offset==0) {
		list.push_back(std::make_pair(row, offset));
		++offset;
	} else if (offset==1) {
		list.push_back(std::make_pair(row, offset));
		++it.first; // increment to next row
		++offset;
	} else {
		throw std::runtime_error("should not happen");
	}

	return list.back();
}

SynapseRowManager::Assignment const&
SynapseRowManager::get(HMF::Coordinate::VLineOnHICANN const& vline) const
{
	return mAllocation.at(vline);
}

std::pair<HMF::HICANN::DriverDecoder, Projection::synapse_type>
SynapseRowManager::fromIndex(size_t idx) const
{
	if (idx>=(2 /*exc/inh*/ * 4 /*MSBs*/)) {
		throw std::runtime_error("array index out of range");
	}

	return { HMF::HICANN::DriverDecoder(idx%4),
		(idx<4) ? "excitatory" : "inhibitory" };
}

void SynapseRowManager::check(size_t const max_chain_length)
{
	for (auto const& entry : mAllocation)
	{
		Assignment const& assign = entry.second;

		size_t cnt = 0;
		for (auto const& v : assign) {
			cnt += v.size();
		}

		// factors of two:
		//	* two insertion points
		//	* two rows per driver
		//	* two independen assignments for odd and even synapse columns.
		//if (cnt>max_chain_length*2*2*2) {
			//throw std::runtime_error("too many synrows assigned");
		//}
	}
}

size_t SynapseRowManager::array_index(
	HMF::HICANN::DriverDecoder const& addr,
	Projection::synapse_type const& type)
{
	size_t res = 0;
	if (type == "excitatory") {
		res = addr;
	} else if (type == "inhibitory") {
		res = HMF::HICANN::DriverDecoder::end+addr;
	} else {
		throw std::runtime_error("unknown synapse target type");
	}

	if (res>=(2 /*exc/inh*/ * 4 /*MSBs*/)) {
		throw std::runtime_error("array index out of range");
	}
	return res;
}

std::ostream& operator<< (std::ostream& os, SynapseRowManager const& mgr)
{
	os << "SynapseRowManager allocations:" << std::endl;
	for (auto const& entry : mgr.mAllocation) {
		auto const& vline = entry.first;
		os << "    " << vline << " " << mgr.mLines.at(vline) << ": ";
		for (size_t ii=0; ii<entry.second.size(); ++ii) {
			os << " " << entry.second[ii].size();
		}
		os << std::endl;
	}
	return os;
}


/// manages the lookup of weights(hw-synapses) belonging to the same bin (2MSB,exc/inh)
/// and target neuron
/// If the weight ref for this sub row has not been initialized,
/// updated ref.col to rows[ref.row].second
/// If col is positive in smaller than the hw_neuron_width, and row index is within range, the weight lookup is ok.
/// If ref.col is larger than hw_neuron_width, we try to take the next assigne subrow.
/// 
/// returns 0, if the weight ref is ok or was successfully updated
/// returns 1, if there is no more weight available in the assigne sub rows.
int weightLookup(WeightRef& ref, SynapseRowManager::SubRows const& rows, size_t const hw_neuron_width)
{
	if (size_t(ref.row)<rows.size() && size_t(ref.col)<hw_neuron_width) {
		// don't correct the weight lookup, we have
		// a perfectly well reference.
	} else if (size_t(ref.row)<rows.size() && ref.col<0) {
		// uninitialized reference with `ref.col==-1`jjjj
		ref.col=rows[ref.row].second;
	} else if (size_t(ref.col)>=hw_neuron_width && size_t(ref.row+1)<rows.size()) {
		// we already have another row allocated for
		// this source unused by this target. so take it.
		ref.row++;
		ref.col=rows[ref.row].second;
	} else {
		return 1;
	}
	return 0;
}




} // anonymous




SynapseRouting::SynapseRouting(
	HICANNGlobal const& hicann,
	boost::shared_ptr<SynapseLoss> const& sl,
	pymarocco::PyMarocco const& pymarocco,
	graph_t const& graph,
	routing_graph const& routing_graph,
	resource_manager_t const& mgr,
	hardware_system_t& hw) :
		mPyMarocco(pymarocco),
		mHICANN(hicann),
		mHW(hw),
		mGraph(graph),
		mRoutingGraph(routing_graph),
		mManager(mgr),
		mSynapseLoss(sl)
{}

void SynapseRouting::run(placement::Result const& placement,
						 std::vector<LocalRoute> const& route_list)
{
	placement::NeuronPlacementResult const& nrnpl = placement.neuron_placement;
	auto const& revmap = nrnpl.placement();
	auto& chip = mHW[hicann()];

	// firstly set all SynapseDecoders to 0bXX0001 addresses
	invalidateSynapseDecoder();
	tagDefectSynapses();

	// secondly, generate statistics about SynapseDriver requirements
	SynapseDriverRequirements drivers_required(hicann(), nrnpl);
	std::unordered_map<VLineOnHICANN, SynapseDriverRequirements::Histogram> synapse_histogram;
	std::unordered_map<VLineOnHICANN, SynapseDriverRequirements::Histogram> synrow_histogram;
	for (auto const& local_route : route_list)
	{
		VLineOnHICANN const vline(local_route.targetBus(mRoutingGraph).getBusId());
		SynapseDriverRequirements::Histogram& synhist = synapse_histogram[vline];
		SynapseDriverRequirements::Histogram& rowhist = synrow_histogram[vline];

		auto const needed = drivers_required.calc(local_route.route().projections(), mGraph, synhist, rowhist);
		MAROCCO_DEBUG("route " << vline << " requires " << needed.first << " SynapseDrivers");
		set(local_route, needed);
	}

	// in the following, find synapse driver assignments for driver on left and
	// right side.

	// generate two lists, of LocalRoutes seperated into vlines on left and
	// right side of hicann.
	std::array<DriverConfigurationState::IntervalList, 2> lists;

	// two mappings of VLines to route iterators, for drivers on left and right
	// side.
	std::array<std::unordered_map<VLineOnHICANN, std::vector<LocalRoute>::const_iterator>, 2> maps;

	for (auto it=route_list.begin(); it!=route_list.end(); ++it)
	{
		LocalRoute const& local_route = *it;
		L1Bus const& l1 = local_route.targetBus(mRoutingGraph);
		HICANNGlobal const& h = l1.hicann();
		VLineOnHICANN const& vline = l1.toVLine();

		auto const needed = get(local_route);

		// this can only happen in very rare ocasions, where by chance, no
		// weight has been realized in the projectionView. More likely for lower
		// connection probs.
		if (!needed.first) {
			warn(this) << "no synapse driver needed";
			handleSynapseLoss(local_route, nrnpl);
			continue;
		}

		bool side;
		if (h != hicann()) {
			// input from adjacent HICANN
			side = vline<VLineOnHICANN::end/2;
		} else {
			// input from same HICANN
			side = vline.toSideHorizontal();
		}

		lists[side].push_back(DriverInterval(vline, needed.first, needed.second));
		auto res = maps[side].insert(std::make_pair(vline, it));
		if (!res.second) {
			throw std::runtime_error("insertion error");
		}
	}


	size_t const chain_length = mPyMarocco.routing.syndriver_chain_length;
	if (chain_length != 56) {
		warn(this) << "using non-default synapse driver chain length of "
			<< chain_length;
	}


	// then optimize synapse driver assignment
	for (auto const& side : iter_all<SideHorizontal>())
	{
		// if list is empty, there is nothing to optimize
		DriverConfigurationState::IntervalList const& list = lists[side];
		if (list.empty()) {
			continue;
		}

		// do the actual optimization. Use simulated annealing to minimize the
		// synapse driver overlap in favor of larger assignments.
		//DriverConfigurationState state(list, side, chain_length, 42[>seed<]);
		//Annealer<DriverConfigurationState> annealer(state);
		//annealer.run(60000);

		//MAROCCO_DEBUG(annealer.iterations() << " annealing iterations");

		//DriverConfigurationState result_state = *annealer.get();
		//std::vector<VLineOnHICANN> const rejected = result_state.postProcess();
		//auto const result = result_state.result(hicann());

		// prepare defect list first.
		std::vector<SynapseDriverOnHICANN> defect_list;
		auto const& defects = mManager.get(hicann());
		auto const& drvs = defects->drivers();
		for (auto it=drvs->begin_disabled(); it!=drvs->end_disabled(); ++it)
		{
			defect_list.push_back(*it);
			MAROCCO_INFO("Marked " << *it << " on " << hicann() << " as defect/disabled");
		}

		Fieres fieres(list, side, chain_length, defect_list);
		std::vector<VLineOnHICANN> const rejected = fieres.rejected();
		auto const result = fieres.result(hicann());


		// mark synapses lost beloning to rejected incoming routes
		for (auto const& vline : rejected)
		{
			LocalRoute const& local_route = *maps[side].at(vline);
			handleSynapseLoss(local_route, nrnpl);
		} // for all rejected


		// lookup table for efficient (syn_row, route) and fast weight
		// assignment.
		std::unordered_map<VLineOnHICANN, std::array<std::array<WeightRef, 4*2>, 256>> weight_lookup;


		// manages the SynapseRows assigned to local routes but it
		// differentiates between sources with different MSBs and different
		// synapse types (exc/inh).
		SynapseRowManager row_manager(result);
		row_manager.init(synapse_histogram, synrow_histogram);

		for (auto const& entry : result)
		{
			VLineOnHICANN const& vline = entry.first;
			std::vector<DriverAssignment> const& as = entry.second;
			LocalRoute const& local_route = *maps[side].at(vline);

			////////////////////////////////////////
			// S T O R E   D R I V E R   C O N F I G
			////////////////////////////////////////

			// generate routing result for the current local_route
			DriverResult driver_res(vline);

			// first insert primary and adjacent drivers
			for (DriverAssignment const& da : as)
			{
				std::vector<SynapseDriverOnHICANN>& drivers = driver_res.drivers()[da.primary];
				drivers.reserve(da.drivers.size());
				for (auto const& drv : da.drivers) {
					drivers.push_back(drv);
				}
			}

			row_manager.check(chain_length);

			// insert synapse row configuration
			auto const& syn_row_assignment = row_manager.get(vline);
			for (size_t ii=0; ii<syn_row_assignment.size(); ++ii)
			{
				SynapseRowManager::SubRows const& rows = syn_row_assignment[ii];
				if (rows.empty()) {
					continue;
				}

				DriverDecoder dec;
				Projection::synapse_type type;
				std::tie(dec, type) = row_manager.fromIndex(ii);

				for(auto const& row : rows)
				{
					auto it = driver_res.rows().find(row.first);
					if (it == driver_res.rows().end()) {
						// insert into result
						bool success = false;
						std::tie(it, success) = driver_res.rows().insert(
							std::make_pair(row.first, SynapseRowSource(type)));
						if (!success) {
							throw std::runtime_error("unable to insert RowSource");
						}
					}
					it->second.prefix(row.second) = mPyMarocco.only_bkg_visible ? DriverDecoder(0) : dec;
				}
			}

			//////////////////////////////////
			// P R O C E S S   S Y N A P S E S
			//////////////////////////////////

			auto& _weight_lookup = weight_lookup[vline];



			//size_t num_sources = local_route.numSources();
			//Route::BusSegment targetBus = local_route.targetBus(); // unecessary here
			Route const& route = local_route.route();
			L1Bus const& l1 = mRoutingGraph[route.source()];
			HICANNGlobal const& src_hicann = l1.hicann();

			std::vector<HardwareProjection> const& projections = route.projections();

			for (auto const& proj : projections)
			{
				graph_t::edge_descriptor pynn_proj = proj.projection();

				graph_t::vertex_descriptor target = boost::target(pynn_proj, mGraph);

				assignment::AddressMapping const& am = proj.source();
				std::vector<L1Address> const& addresses = am.addresses();

				assignment::PopulationSlice const& src_bio_assign = am.bio();

				size_t const src_bio_size   = src_bio_assign.size();
				size_t const src_bio_offset = src_bio_assign.offset();


				// now there is everything from the source, now need more info about target
				// population placement.


				boost::shared_ptr<ProjectionView> const proj_view =
					boost::make_shared<ProjectionView>(mGraph[pynn_proj]);
				Population const& target_pop = *mGraph[target];


				// calculate offsets for pre population in this view
				size_t const src_neuron_offset_in_proj_view =
					getPopulationViewOffset(src_bio_offset, proj_view->pre().mask());

				// get synloss proxy object for faster loss counting
				SynapseLossProxy syn_loss_proxy =
					mSynapseLoss->getProxy(pynn_proj, src_hicann, hicann());

				std::vector<assignment::NeuronBlockSlice> const& hwassigns = revmap.at(target);
				for (auto const& hwassign: hwassigns)
				{
					auto const& terminal = hwassign.coordinate();
					if (terminal.toHICANNGlobal() != hicann()) {
						// this terminal doesn't correspond to the current local
						// hicann. so there is nothing to do.
						continue;
					}

					placement::OnNeuronBlock const& onb = nrnpl.at(
					    terminal.toHICANNGlobal())[terminal.toNeuronBlockOnHICANN()];
					auto const it = onb.get(hwassign.offset());
					assert(it != onb.end());

					{
						// FIXME: Confirm and remove this:
						NeuronOnNeuronBlock first = *onb.neurons(it).begin();
						assert(first == hwassign.offset());
					}

					NeuronOnNeuronBlock const& first = hwassign.offset();
					std::shared_ptr<placement::NeuronPlacement> const& trg_assign = *it;
					assignment::PopulationSlice const& trg_bio_assign = trg_assign->population_slice();

					size_t const trg_bio_size   = trg_bio_assign.size();
					size_t const trg_bio_offset = trg_bio_assign.offset();
					// FIXME: first.toNeuronOnHICANN(nb)...!
					size_t const trg_pop_hw_offset =
					    terminal.toNeuronBlockOnHICANN().value() * 32 + first.x();

					size_t const hw_neuron_width = trg_assign->neuron_width();

					auto bio_weights = proj_view->getWeights(); // this is just a view, no copying

					// calculate offsets for post population in this view
					size_t const trg_neuron_offset_in_proj_view =
						getPopulationViewOffset(trg_bio_offset, proj_view->post().mask());

					// TODO: switch back to iterator later on. But for now
					// counting the neuron indexes and indexing the matrix
					// every time is safer and easier to debug.
					//boost::numeric::ublas::matrix_row<Connector::const_matrix_view_type> row(bio_weights, src_neuron_in_proj_view);
					//auto weight_iterator = row.begin()+trg_neuron_offset_in_proj_view ;
					size_t trg_neuron_in_proj_view = trg_neuron_offset_in_proj_view;
					for (size_t trg_neuron=trg_bio_offset; trg_neuron<trg_bio_offset+trg_bio_size; ++trg_neuron)
					{
						if (!proj_view->post().mask()[trg_neuron]) {
							continue;
						}


						size_t const trg_hw_offset = trg_pop_hw_offset+(trg_neuron-trg_bio_offset)*hw_neuron_width;

						size_t address_cnt = 0;
						size_t src_neuron_in_proj_view = src_neuron_offset_in_proj_view;
						for (size_t src_neuron=src_bio_offset; src_neuron<src_bio_offset+src_bio_size; ++src_neuron)
						{
							size_t address = address_cnt++;
							// source neuron address (relative to Population) is
							// ONLY needed for mask. To address weights, always use
							// neuron address relative to PopulationView.
							if (!proj_view->pre().mask()[src_neuron]) {
								continue;
							}

							L1Address const& l1_address = addresses.at(address);
							DriverDecoder const driver_dec = l1_address.getDriverDecoderMask();

							WeightRef& ref = _weight_lookup[trg_hw_offset][
								SynapseRowManager::array_index(driver_dec,
									proj_view->projection()->target())];


							//double const weight = *(weight_iterator++);
#if !defined(MAROCCO_NDEBUG)
							//if (src_neuron_in_proj_view>=bio_weights.size1()) {
								//throw std::runtime_error("src out of range");
							//}
							//if (trg_neuron_in_proj_view>=bio_weights.size2()) {
								//throw std::runtime_error("trg out of range");
							//}
#endif // MAROCCO_NDEBUG
							double weight = bio_weights(src_neuron_in_proj_view, trg_neuron_in_proj_view);
							if (weight>0.)
							{
								auto const& rows = row_manager.getRows(vline, driver_dec,
									proj_view->projection()->target());

								// try to find hw synapse in assigned subrows

								int ret = 0; // 0: weight lookup successful, 1: no synapse left
								int cont = 1; // continue searching
								while (ret==0 && cont)
								{
									ret = weightLookup(ref, rows, hw_neuron_width);
									if (ret) {
										break;
									}

									size_t const column = trg_hw_offset + ref.col;
									auto const& syn_row = rows[ref.row];

									auto proxy = chip.synapses[syn_row.first];

									// assert that synapse has not been used otherwise
									assert(proxy.decoders[column]==SynapseDecoderDisablingSynapse);

									// check that synapse has not been tagged as defect
									if (proxy.weights[column]==SynapseWeight(0)) {
										//  we found usable weight
										cont = 0;
									} else {
										ref.col+=2;
									}
								}

								if (ret) {
									// we have no more synapse rows, so add all
									// the rest to synapse loss.
									MAROCCO_TRACE("handling synapse loss");
									for (; src_neuron<src_bio_offset+src_bio_size; ++src_neuron)
									{
										L1Address const& l1_address_ = addresses.at(address++);
										if (!proj_view->pre().mask()[src_neuron]) {
											continue;
										}

										// we steped onto a source with different MSBs
										if (driver_dec!=l1_address_.getDriverDecoderMask()) {
											// update address_cnt value to jump
											// beyond the bin of sources with
											// equal MSB.
											address_cnt = address-1;
											src_neuron--;
											break;
										}

										double const weight = bio_weights(src_neuron_in_proj_view,
																		  trg_neuron_in_proj_view);
										if (weight>0.) {
											syn_loss_proxy.addLoss(src_neuron_in_proj_view, trg_neuron_in_proj_view);
										}

										++src_neuron_in_proj_view;
									}

									// there might be nothing more to do, we use `continue` because, if
									// `src_neuron` at end we will jump to next target anyway
									continue;
								}


								size_t const column = trg_hw_offset + ref.col;
								auto const& syn_row = rows[ref.row];

								auto proxy = chip.synapses[syn_row.first];
#ifndef MAROCCO_NDEBUG
								if (proxy.decoders[column] != SynapseDecoderDisablingSynapse) {
									MAROCCO_ERROR("row: " << ref.row << " col: " << ref.col);
									throw std::runtime_error("lookup error");
								}
#endif // MAROCCO_NDEBUG

								// we found a usable weight
								MAROCCO_TRACE("setting synapse (" << syn_row.first << ", "
									  << syn_row.second<< ", " <<column << ") = "
									  << l1_address.getSynapseDecoderMask());
								proxy.decoders[column] = mPyMarocco.only_bkg_visible ? SynapseDecoder(0) : l1_address.getSynapseDecoderMask();

								// Before, here the distorted weight was stored.
								// As we postpone the weight trafo to HICANNTransformator, this has to be done there (TODO)
								// Instead, we only mark the synapse as realized in the target chip.
								//syn_loss_proxy.setWeight(src_neuron_in_proj_view, trg_neuron_in_proj_view, clipped_weight);
								syn_loss_proxy.addRealized();

								// store synapse mapping
								auto it = driver_res.rows().find(syn_row.first);
								assert (it != driver_res.rows().end());
								it->second.synapses().at(column) = SynapseSource( proj_view, src_neuron_in_proj_view, trg_neuron_in_proj_view);

								// update weight ref for next synapse within bin
								ref.col+=2;
							} // if valid bio weights
							++src_neuron_in_proj_view;
						} // src bio neurons
						++trg_neuron_in_proj_view;
					} // trg bio neurons

				} // all hw assignments
			} // all hw projections


			mResult.push_back(std::move(driver_res));

		} // synapse driver assignments

		MAROCCO_DEBUG(row_manager); // print row usage stats

	} // left/right driver bank

	disableDefectSynapes();
}

void SynapseRouting::set(LocalRoute const& route, std::pair<size_t, size_t> const& need)
{
	mNumSynapseDriver[route.id()] = need;
}

std::pair<size_t, size_t> SynapseRouting::get(LocalRoute const& route) const
{
	return mNumSynapseDriver.at(route.id());
}

SynapseRouting::Result const& SynapseRouting::getResult() const
{
	return mResult;
}

SynapseRouting::Result& SynapseRouting::getResult()
{
	return mResult;
}

void SynapseRouting::handleSynapseLoss(LocalRoute const& local_route,
									   placement::NeuronPlacementResult const& nrnpl)
{
	placement::PlacementMap const& revmap = nrnpl.placement();

	Route const& route = local_route.route();
	L1Bus const& l1 = mRoutingGraph[route.source()];
	HICANNGlobal const& src_hicann = l1.hicann();
	std::vector<HardwareProjection> const& projections = route.projections();
	for (auto const& proj : projections)
	{
		graph_t::edge_descriptor pynn_proj = proj.projection();

		graph_t::vertex_descriptor target = boost::target(pynn_proj, mGraph);

		assignment::AddressMapping const& am = proj.source();
		assignment::PopulationSlice const& src_bio_assign = am.bio();

		std::vector<assignment::NeuronBlockSlice> const& hwassigns = revmap.at(target);
		for (auto const& hwassign : hwassigns) {
			auto const& terminal = hwassign.coordinate();
			if (terminal.toHICANNGlobal() != hicann()) {
				// this terminal doesn't correspond to the current local
				// hicann. so there is nothing to do.
				continue;
			}

			placement::OnNeuronBlock const& onb =
			    nrnpl.at(terminal.toHICANNGlobal())[terminal.toNeuronBlockOnHICANN()];
			auto trg_bio_assign = onb[hwassign.offset()]->population_slice();

			mSynapseLoss->addLoss(pynn_proj, src_hicann, terminal.toHICANNGlobal(),
								  src_bio_assign, trg_bio_assign);
		} // for all asssignments
	} // for all projetions
}

void SynapseRouting::invalidateSynapseDecoder()
{
	auto& chip = mHW[hicann()];

	for (auto const& row : iter_all<SynapseRowOnHICANN>())
	{
		auto proxy = chip.synapses[row];
		for (auto& dec : proxy.decoders) {
			// 0bXX0001 addresses are unused addresses
			dec = SynapseDecoderDisablingSynapse;
		}
	}
}

void SynapseRouting::tagDefectSynapses()
{
	auto& chip = mHW[hicann()];
	auto const& defects = mManager.get(hicann());
	auto const& syns = defects->synapses();

	for (auto it=syns->begin_disabled(); it!=syns->end_disabled(); ++it)
	{
		auto proxy = chip.synapses[*it];
		proxy.weight = SynapseWeight(1);
		MAROCCO_INFO("Marked " << *it << " on " << hicann() << " as defect/disabled");
	}
}

void SynapseRouting::disableDefectSynapes()
{
	auto& chip = mHW[hicann()];

	for (auto const& syn : iter_all<SynapseOnHICANN>())
	{
		auto proxy = chip.synapses[syn];
		if (proxy.decoder == SynapseDecoderDisablingSynapse) {
			proxy.weight = SynapseWeight(0);
		}
	}
}

} // namespace routing
} // namespace marocco
