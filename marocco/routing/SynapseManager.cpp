#include "marocco/routing/SynapseManager.h"
#include "hal/Coordinate/iter_all.h"
#include "marocco/routing/print_tuple.h"

using namespace HMF::Coordinate;
using HMF::HICANN::DriverDecoder;

namespace marocco {
namespace routing {

SynapseManager::SynapseManager(Result const& resources) : mAllocation(), mLines()
{
	for (auto const& entry : resources) {
		VLineOnHICANN const& vline = entry.first;
		std::vector<DriverAssignment> const& vec = entry.second;

		mLines[vline] = 0;
		for (auto const& da : vec) {
			mLines[vline] += da.drivers.size();
			for (auto const& d : da.drivers)
				mDrivers[vline].push_back(d);
		}
		assert(mLines[vline] == mDrivers[vline].size());
	}
}

void SynapseManager::init(HistMap const& synapse_hist, HistMap const& synrow_hist)
{
	for (auto const& entry : mLines) {
		VLineOnHICANN const& vline = entry.first;
		size_t avail_drivers = entry.second;
		Histogram const& synrow_histogram = synrow_hist.at(vline);
		Histogram const& synapse_histogram = synapse_hist.at(vline);

		// TODO: ensure that both Histograms have the same keys.

		/////////////////////////////////////////////////////////////////////////
		// Calculate requirements and synapse counts for different hierarchies //
		/////////////////////////////////////////////////////////////////////////
		//
		// mmh, we re-do the calculation of rows per driver. This could be passed to here from the
		// Syndr Req.
		// 1) Total nr of drivers
		// 2) Nr of drivers per STP
		// 3) Nr of rows per Side
		// 4) Nr of half rows per hw-property;

		// 0) build hierarchical maps with single keys instead of tuples.
		typedef std::map<STPMode,
						 std::map<Side, std::map<Parity, std::map<DriverDecoder, size_t> > > >
			PerDecoderMap;

		typedef std::map<STPMode, std::map<Side, size_t> > PerSideMap;
		typedef std::map<STPMode, size_t> PerSTPMap;

		PerDecoderMap half_rows;
		PerDecoderMap synapses;

		PerSideMap rows_per_side;
		PerSideMap synapses_per_side;

		PerSTPMap drivers_per_stp;
		PerSTPMap synapses_per_stp;

		for (auto const& item : synrow_histogram) {
			Side side;
			Parity parity;
			DriverDecoder decoder;
			STPMode stp;
			std::tie(side, parity, decoder, stp) = item.first;
			size_t const half_row_count = item.second;
			size_t const syn_count = synapse_histogram.at(item.first);

			half_rows[stp][side][parity][decoder] = half_row_count;
			synapses[stp][side][parity][decoder] = syn_count;
			synapses_per_side[stp][side] += syn_count;
			synapses_per_stp[stp] += syn_count;
		}

		// 1-4) calc requirements
		size_t r_drivers = 0; // number of required drivers
		for (auto const& stp_item : half_rows) {
			STPMode const& stp = stp_item.first;
			size_t req_rows_per_stp = 0;
			for (auto const& side_item : stp_item.second) {
				Side const& side = side_item.first;
				// The number of required rows per side is maximum of half rows per parity;
				size_t req_rows_per_side = 0;
				for (auto const& parity_item : side_item.second) {
					size_t req_half_rows = std::accumulate(
						parity_item.second.begin(), parity_item.second.end(), 0,
						map_acc<DriverDecoder, size_t>());
					if (req_half_rows > req_rows_per_side)
						req_rows_per_side = req_half_rows;
				}
				rows_per_side[stp][side] = req_rows_per_side;
				req_rows_per_stp += req_rows_per_side;
			}
			size_t const req_drivers_per_stp =
				size_t(std::ceil(req_rows_per_stp / 2.)); // 2 rows per driver
			drivers_per_stp[stp] = req_drivers_per_stp;
			r_drivers += req_drivers_per_stp;
		}

		//////////////////////////////////////////////////////////////////////
		// Assign available Drivers, Rows, Subrows to HW Synapse Properties //
		//////////////////////////////////////////////////////////////////////

		// 1.) Drivers per STP
		PerSTPMap assigned_drivers_per_stp =
			relative_reduction(drivers_per_stp, synapses_per_stp, avail_drivers);

		// 2.) Rows per Side
		PerSideMap assigned_rows_per_side;

		for (auto const& stp_item : rows_per_side) {
			STPMode const& stp = stp_item.first;
			size_t assigned_rows = assigned_drivers_per_stp[stp] * 2;
			assigned_rows_per_side[stp] =
				relative_reduction(stp_item.second, synapses_per_side[stp], assigned_rows);
		}

		// 3.) Half rows per decoder
		PerDecoderMap assigned_half_rows;

		for (auto const& stp_item : half_rows) {
			STPMode const& stp = stp_item.first;
			for (auto const& side_item : stp_item.second) {
				Side const& side = side_item.first;
				// nr of assigned rows is equal to number of assigned half rows per parity
				size_t assigned_rows = assigned_rows_per_side[stp][side];
				for (auto const& parity_item : side_item.second) {
					Parity const& parity = parity_item.first;
					assigned_half_rows[stp][side][parity] = relative_reduction(
						parity_item.second, synapses[stp][side][parity], assigned_rows);
				}
			}
		}

		//////////////////////////////////////////////
		// Allocate concrete Drivers, Rows, SubRows //
		//////////////////////////////////////////////

		Assignment& assignment = mAllocation[vline];

		std::list<SynapseDriverOnHICANN>& real_drivers = mDrivers[vline];

		for (auto const& stp_item : assigned_half_rows) {
			STPMode const& stp = stp_item.first;

			// allocate drivers and rows
			std::list<SynapseDriverOnHICANN> allocated_drivers;
			size_t const assigned_drivers = assigned_drivers_per_stp[stp];
			for (size_t i = 0; i < assigned_drivers; ++i) {
				allocated_drivers.push_back(real_drivers.front());
				real_drivers.pop_front();
			}

			std::list<SynapseRowOnHICANN> allocated_rows;
			for (auto const& ad : allocated_drivers) {
				for (auto const& row : iter_all<RowOnSynapseDriver>()) {
					allocated_rows.push_back(SynapseRowOnHICANN(ad, row));
				}
			}
			for (auto const& side_item : stp_item.second) {
				Side const& side = side_item.first;
				size_t const assigned_rows = assigned_rows_per_side[stp][side];
				std::list<SynapseRowOnHICANN> my_rows;
				for (size_t i = 0; i < assigned_rows; ++i) {
					my_rows.push_back(allocated_rows.front());
					allocated_rows.pop_front();
				}

				for (auto const& parity_item : side_item.second) {
					Parity const& parity = parity_item.first;
					std::list<SynapseRowOnHICANN> my_rows_parity = my_rows; // copy!
					for (auto const& decoder_item : parity_item.second) {
						DriverDecoder const& decoder = decoder_item.first;
						size_t const count = decoder_item.second;
						Side_Parity_Decoder_STP const hw_synapse_property(
							side, parity, decoder, stp);
						for (size_t i = 0; i < count; ++i) {
							auto const synrow_c = my_rows_parity.front();
							my_rows_parity.pop_front();
							assignment[hw_synapse_property].push_back(synrow_c);
						}
					}
					// TODO: store unused half rows for debug
				} // even and odd columns
			}	 // left and right synaptic inputs
		}		  // for all stp settings
	}			  // for all synapse driver assignments on this side
}

std::ostream& operator<<(std::ostream& os, SynapseManager const& mgr)
{
	os << "SynapseManager allocations:" << std::endl;
	for (auto const& entry : mgr.mAllocation) {
		auto const& vline = entry.first;
		os << "    " << vline << " " << mgr.mLines.at(vline) << ": ";
		for (auto const& item2 : entry.second) {
			os << item2.first << " " << item2.second.size() << "\n";
		}
		os << std::endl;
	}
	return os;
}


SynapseManager::SubRows const& SynapseManager::getRows(
	HMF::Coordinate::VLineOnHICANN const& vline, Side_Parity_Decoder_STP const& hw_synapse_property)
{
	return mAllocation[vline][hw_synapse_property];
}

SynapseManager::Assignment const&
SynapseManager::get(HMF::Coordinate::VLineOnHICANN const& vline) const
{
	return mAllocation.at(vline);
}

void SynapseManager::check(size_t chain_length)
{
	for (auto const& entry : mAllocation) {
		Assignment const& assign = entry.second;

		// factors of two:
		//	* two insertion points
		//	* two rows per driver
		//	* two independen assignments for odd and even synapse columns.
		// if (cnt>max_chain_length*2*2*2) {
		// throw std::runtime_error("too many synrows assigned");
		//}
	}
}

SynapseManager::SynapsesOnVLine SynapseManager::getSynapses(
	HMF::Coordinate::VLineOnHICANN const& vline,
	std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
					   std::map<SynapseType, SynapseColumnsMap> > const& synapse_columns)
{
	// assure that SynapsesOnVLine is only created once per vline
	bool inserted;
	std::tie(std::ignore, inserted) = mSynapsesOnVLineCreated.insert(vline);
	if (!inserted) {
		std::stringstream ss;
		ss << "getSynapses() called twice for vline " << vline;
		throw std::runtime_error(ss.str());
	}
	return SynapsesOnVLine(synapse_columns, get(vline));
}

SynapseManager::SynapseStepper::SynapseStepper(
	Assignment const& assignment, Decoder_STP dec_stp, SynapseColumnsMap const& s_p_to_c_map)
	: hw_prop_to_half_row(assignment),
	  decoder_stp(dec_stp),
	  side_parity_to_columns_map(s_p_to_c_map)
{
	_has_synapses = false;

	for (auto const& item : s_p_to_c_map)
		assert(item.second.size()); // Assure that there are only side parity entries with synapse
									// columns

	side_parity_it = side_parity_to_columns_map.begin();
	while (side_parity_it != side_parity_to_columns_map.end()) {
		Side side;
		Parity parity;
		DriverDecoder decoder;
		STPMode stp;
		std::tie(side, parity) = side_parity_it->first;
		std::tie(decoder, stp) = decoder_stp;
		auto it = hw_prop_to_half_row.find(Side_Parity_Decoder_STP(side, parity, decoder, stp));
		if (it != hw_prop_to_half_row.end()) {
			hw_prop_to_half_row_it = it;
			sub_row_it = hw_prop_to_half_row_it->second.begin();
			if (sub_row_it != hw_prop_to_half_row_it->second.end()) {
				syn_col_it = side_parity_it->second.begin();
				_has_synapses = true;
				break;
			}
		}
		side_parity_it++;
	}
}

HMF::Coordinate::SynapseOnHICANN SynapseManager::SynapseStepper::get()
{
	if (!_has_synapses)
		throw std::runtime_error("no synapse left");
	HMF::Coordinate::SynapseOnHICANN rv(*sub_row_it, *syn_col_it);
	update();
	return rv;
}

void SynapseManager::SynapseStepper::update()
{
	if (!_has_synapses)
		return;
	_has_synapses = false;
	// try next syn col
	syn_col_it++;
	if (syn_col_it != side_parity_it->second.end()) {
		_has_synapses = true;
	} else {
		// next synapse row
		sub_row_it++;
		if (sub_row_it != hw_prop_to_half_row_it->second.end()) {
			syn_col_it = side_parity_it->second.begin();
			_has_synapses = true;
		} else {
			// try next side parity option
			side_parity_it++;
			while (side_parity_it != side_parity_to_columns_map.end()) {
				Side side;
				Parity parity;
				DriverDecoder decoder;
				STPMode stp;
				std::tie(side, parity) = side_parity_it->first;
				std::tie(decoder, stp) = decoder_stp;
				auto it =
					hw_prop_to_half_row.find(Side_Parity_Decoder_STP(side, parity, decoder, stp));
				if (it != hw_prop_to_half_row.end()) {
					hw_prop_to_half_row_it = it;
					sub_row_it = hw_prop_to_half_row_it->second.begin();
					if (sub_row_it != hw_prop_to_half_row_it->second.end()) {
						syn_col_it = side_parity_it->second.begin();
						_has_synapses = true;
						break;
					}
				}
				side_parity_it++;
			}
		}
	}
}

SynapseManager::SynapsesOnVLine::SynapsesOnVLine(
	std::unordered_map<HMF::Coordinate::NeuronOnHICANN, TypeToSynapsesColumnsMap> const&
		TypeToSynapseColumnsMapPerNeuron,
	Assignment const& assignment)
	: mTypeToSynapseColumnsMapPerNeuron(TypeToSynapseColumnsMapPerNeuron),
	  mAssignedHalfRows(assignment)
{
}


std::pair<HMF::Coordinate::SynapseOnHICANN, bool> SynapseManager::SynapsesOnVLine::getSynapse(
	HMF::Coordinate::NeuronOnHICANN const& target_nrn, Type_Decoder_STP const& bio_synapse_property)
{
	std::map<Type_Decoder_STP, SynapseStepper>& bio_synapses_on_target_neuron =
		mBioSynapsesPerNeuron[target_nrn];

	auto it = bio_synapses_on_target_neuron.find(bio_synapse_property);
	if (it == bio_synapses_on_target_neuron.end()) {
		SynapseType type;
		DriverDecoder decoder;
		STPMode stp;
		std::tie(type, decoder, stp) = bio_synapse_property;
		it = bio_synapses_on_target_neuron.insert(
											  {bio_synapse_property,
											   SynapseStepper(
												   mAssignedHalfRows, Decoder_STP(decoder, stp),
												   mTypeToSynapseColumnsMapPerNeuron.at(target_nrn)
													   .at(type))})
				 .first;
	}

	std::pair<HMF::Coordinate::SynapseOnHICANN, bool> rv;
	rv.second = it->second.has_synapses();
	if (rv.second)
		rv.first = it->second.get();
	return rv;
}


} // namespace routing
} // namespace marocco
