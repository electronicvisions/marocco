#pragma once

#include <vector>
#include <array>
#include <iosfwd>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <boost/icl/interval_map.hpp>
#include <boost/shared_ptr.hpp>

#include "marocco/config.h"
#include "marocco/annealer.h"
#include "marocco/RNG.h"
#include "marocco/routing/DriverAssignment.h"
#include "marocco/routing/SynapseDriverSA.h"

namespace marocco {
namespace routing {

/// @brief BinPacking Synapse Driver Routing implementation.
///
/// Similar to original fieres appeoach.
class Fieres
{
public:
	typedef HMF::Coordinate::VLineOnHICANN VLineOnHICANN;
	typedef std::vector<DriverInterval> IntervalList;
	typedef std::unordered_map<
			VLineOnHICANN,
			std::vector<DriverAssignment>
		> Result;
	typedef std::vector<VLineOnHICANN> Rejected;

	Fieres(IntervalList const& list,
		   HMF::Coordinate::Side const& side,
		   size_t max_chain_length=56);

	Fieres(IntervalList const& list,
		   HMF::Coordinate::Side const& side,
		   size_t max_chain_length,
		   std::vector<HMF::Coordinate::SynapseDriverOnHICANN> const& defect);

	Result result(HMF::Coordinate::HICANNGlobal const& hicann) const;
	Rejected rejected() const;

private:
	HMF::Coordinate::Side const mSide;

	Result mResult;
	Rejected mRejected;
};

namespace fieres {

struct InboundRoute {
	InboundRoute() = default;
	InboundRoute(InboundRoute const&) = default;
	InboundRoute(HMF::Coordinate::VLineOnHICANN const& vline, int drv, double syns,
	             size_t ass)
	    : line(vline), drivers(drv), synapses(syns), assigned(ass) {}

	static const int DEFECT;

	HMF::Coordinate::VLineOnHICANN line;

	/// number of requested synapse drivers
	int drivers = -1; // "defect" by default

	/// number of represented synapses
	// FIXME: This is not really used later on?
	double synapses = 0;

	/// number of actually assigned drivers. can be <= this->drivers
	size_t assigned = 0;
};

/// Holds assignment data for all synapse drivers of one side
/// (left/right) of the HICANN.
class Assignment {
	typedef HMF::Coordinate::SynapseDriverOnQuadrant coord_t;
	typedef HMF::Coordinate::SideVertical side_vertical_t;

public:
	typedef std::unordered_map<HMF::Coordinate::VLineOnHICANN,
	                           std::vector<DriverAssignment>> result_t;

	Assignment(HMF::Coordinate::SideHorizontal const& side) : mSide(side) {}

	/// Disable a synapse driver for further use.
	/// Note that you HAVE to mark defects before doing anything else.
	void add_defect(side_vertical_t const& side, coord_t const& drv);

	/// Find an appropriate gap with enough unused adjacent drivers and
	/// insert this incoming route.  Returns true on success and false
	/// if this route is rejected (due to resource shortage).
	bool add(InboundRoute const& route);

	result_t result() const;

protected:
	void insert(side_vertical_t const& side, coord_t const& drv,
	            InboundRoute const& route);

	/// Represents a range of synapse drivers assigned to the same incoming route.
	struct interval_t {
		interval_t() : route(), primary(), begin(0), end(0) {}
		interval_t(InboundRoute val, HMF::Coordinate::SynapseDriverOnQuadrant drv,
		           std::ptrdiff_t b, std::ptrdiff_t e)
		    : route(val), primary(drv), begin(b), end(e) {}

		InboundRoute route;
		HMF::Coordinate::SynapseDriverOnQuadrant primary;
		std::ptrdiff_t begin;
		std::ptrdiff_t end;
	};

	typedef std::shared_ptr<interval_t> value_t;
	typedef std::array<value_t, coord_t::end> array_t;

	static bool unassigned_p(value_t const& val) { return !val; }

	static bool assigned_p(value_t const& val) { return !!val; }

	std::array<array_t, 2> mData;
	HMF::Coordinate::SideHorizontal mSide;
};

} // namespace fieres

} // namespace routing
} // namespace marocco
