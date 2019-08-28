#pragma once

#include <iosfwd>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/icl/interval_map.hpp>
#include <boost/shared_ptr.hpp>

#include "hal/Coordinate/typed_array.h"

#include "marocco/config.h"
#include "marocco/routing/DriverInterval.h"
#include "marocco/routing/results/ConnectedSynapseDrivers.h"

namespace marocco {
namespace routing {

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
	double synapses = 0;

	/// number of actually assigned drivers. can be <= this->drivers
	size_t assigned = 0;
};

/// Holds assignment data for all synapse drivers of one side
/// (left/right) of the HICANN.
class Assignment {
	typedef HMF::Coordinate::SynapseDriverOnQuadrant coordinate_type;

public:
	typedef std::unordered_map<HMF::Coordinate::VLineOnHICANN,
	                           std::vector<results::ConnectedSynapseDrivers> >
		result_type;

	Assignment(HMF::Coordinate::SideHorizontal const& side) : mSide(side) {}

	/// Disable a synapse driver for further use.
	/// Note that you HAVE to mark defects before doing anything else.
	void add_defect(HMF::Coordinate::SideVertical const& side, coordinate_type const& drv);

	/// Find an appropriate gap with enough unused adjacent drivers and
	/// insert this incoming route.  Returns true on success and false
	/// if this route is rejected (due to resource shortage).
	bool add(InboundRoute const& route);

	result_type result() const;

protected:
	void insert(
		HMF::Coordinate::SideVertical const& side,
		coordinate_type const& drv,
		InboundRoute const& route);

	/// Represents a range of synapse drivers assigned to the same incoming route.
	struct interval_type
	{
		interval_type() : route(), primary(), begin(0), end(0)
		{
		}

		interval_type(
			InboundRoute val,
			HMF::Coordinate::SynapseDriverOnQuadrant drv,
			std::ptrdiff_t b,
			std::ptrdiff_t e)
			: route(val), primary(drv), begin(b), end(e)
		{
		}

		InboundRoute route;
		HMF::Coordinate::SynapseDriverOnQuadrant primary;
		std::ptrdiff_t begin;
		std::ptrdiff_t end;
	};

	typedef std::shared_ptr<interval_type> value_type;
	typedef HMF::Coordinate::typed_array<value_type, coordinate_type> array_type;

	static bool unassigned_p(value_type const& val)
	{
		return !val;
	}

	static bool assigned_p(value_type const& val)
	{
		return !!val;
	}

	HMF::Coordinate::typed_array<array_type, HMF::Coordinate::SideVertical> mData;
	HMF::Coordinate::SideHorizontal mSide;
};

} // namespace fieres

/// @brief BinPacking Synapse Driver Routing implementation.
///
/// Similar to original fieres appeoach.
class Fieres
{
public:
	typedef HMF::Coordinate::VLineOnHICANN VLineOnHICANN;
	typedef std::vector<DriverInterval> IntervalList;
	typedef std::unordered_map<VLineOnHICANN, std::vector<results::ConnectedSynapseDrivers> >
	    Result;
	typedef std::vector<VLineOnHICANN> Rejected;

	Fieres(
	    IntervalList const& list,
	    HMF::Coordinate::Side const& side,
	    size_t max_chain_length = HMF::Coordinate::SynapseDriverOnQuadrant::end);

	Fieres(
	    IntervalList const& list,
	    HMF::Coordinate::Side const& side,
	    size_t max_chain_length,
	    std::vector<HMF::Coordinate::SynapseDriverOnHICANN> const& defect);

	Result result() const;
	Rejected rejected() const;

private:
	HMF::Coordinate::Side const mSide;

	Result mResult;
	Rejected mRejected;

	void defrag(
	    std::list<fieres::InboundRoute> const& list,
	    fieres::Assignment& assignment,
	    std::vector<VLineOnHICANN>& rejected);
};

} // namespace routing
} // namespace marocco
