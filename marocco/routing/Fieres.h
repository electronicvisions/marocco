#pragma once

#include <iosfwd>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/icl/interval_map.hpp>
#include <boost/shared_ptr.hpp>

#include "halco/common/typed_array.h"

#include "marocco/config.h"
#include "marocco/routing/DriverInterval.h"
#include "marocco/routing/results/ConnectedSynapseDrivers.h"

namespace marocco {
namespace routing {

namespace fieres {

struct InboundRoute {
	InboundRoute() = default;
	InboundRoute(InboundRoute const&) = default;
	InboundRoute(halco::hicann::v2::VLineOnHICANN const& vline, int drv, double syns,
	             size_t ass)
	    : line(vline), drivers(drv), synapses(syns), assigned(ass) {}

	static const int DEFECT;

	halco::hicann::v2::VLineOnHICANN line;

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
	typedef halco::hicann::v2::SynapseDriverOnQuadrant coordinate_type;

public:
	typedef std::unordered_map<halco::hicann::v2::VLineOnHICANN,
	                           std::vector<results::ConnectedSynapseDrivers> >
		result_type;

	Assignment(halco::common::SideHorizontal const& side) : mSide(side) {}

	/// Disable a synapse driver for further use.
	/// Note that you HAVE to mark defects before doing anything else.
	void add_defect(halco::common::SideVertical const& side, coordinate_type const& drv);

	/// Find an appropriate gap with enough unused adjacent drivers and
	/// insert this incoming route.  Returns true on success and false
	/// if this route is rejected (due to resource shortage).
	bool add(InboundRoute const& route);

	result_type result() const;

protected:
	void insert(
		halco::common::SideVertical const& side,
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
			halco::hicann::v2::SynapseDriverOnQuadrant drv,
			std::ptrdiff_t b,
			std::ptrdiff_t e)
			: route(val), primary(drv), begin(b), end(e)
		{
		}

		InboundRoute route;
		halco::hicann::v2::SynapseDriverOnQuadrant primary;
		std::ptrdiff_t begin;
		std::ptrdiff_t end;
	};

	typedef std::shared_ptr<interval_type> value_type;
	typedef halco::common::typed_array<value_type, coordinate_type> array_type;

	static bool unassigned_p(value_type const& val)
	{
		return !val;
	}

	static bool assigned_p(value_type const& val)
	{
		return !!val;
	}

	halco::common::typed_array<array_type, halco::common::SideVertical> mData;
	halco::common::SideHorizontal mSide;
};

} // namespace fieres

/// @brief BinPacking Synapse Driver Routing implementation.
///
/// Similar to original fieres appeoach.
class Fieres
{
public:
	typedef halco::hicann::v2::VLineOnHICANN VLineOnHICANN;
	typedef std::vector<DriverInterval> IntervalList;
	typedef std::unordered_map<VLineOnHICANN, std::vector<results::ConnectedSynapseDrivers> >
	    Result;
	typedef std::vector<VLineOnHICANN> Rejected;

	Fieres(
	    IntervalList const& list,
	    halco::common::Side const& side,
	    size_t max_chain_length = halco::hicann::v2::SynapseDriverOnQuadrant::end);

	Fieres(
	    IntervalList const& list,
	    halco::common::Side const& side,
	    size_t max_chain_length,
	    std::vector<halco::hicann::v2::SynapseDriverOnHICANN> const& defect);

	Result result() const;
	Rejected rejected() const;

private:
	halco::common::Side const mSide;

	Result mResult;
	Rejected mRejected;

	void defrag(
	    std::list<fieres::InboundRoute> const& list,
	    fieres::Assignment& assignment,
	    std::vector<VLineOnHICANN>& rejected);
};

} // namespace routing
} // namespace marocco
