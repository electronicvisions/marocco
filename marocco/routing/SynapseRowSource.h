#pragma once

#include <ostream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "pywrap/compat/rant.hpp"
#include "pywrap/compat/array.hpp"

#include "marocco/graph.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANN/DriverDecoder.h"

namespace marocco {
namespace routing {

/** represents the mapping source of a hardware synapse, i.e. a bio synapse.
 *  A bio synapse is described by a ProjectionView, and the indices of the source and target
 *  neuron in the view.
 * */
class SynapseSource {

public:
	typedef boost::shared_ptr<ProjectionView> projection_view_type;

	SynapseSource (projection_view_type const & view, size_t src_idx, size_t tgt_idx);
	SynapseSource ();

	projection_view_type const & projection_view() const;
	size_t src() const;
	size_t tgt() const;
	/// returns true, if no synapse is mapped
	bool empty() const;

	bool operator== (SynapseSource const& rhs) const;
	bool operator!= (SynapseSource const& rhs) const;

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);

	/// source neuron in projection view;
    size_t mSrc;
	/// target neuron in projection view;
    size_t mTgt;
	/// projection view
	projection_view_type  mProjView;
};

/// stores the configuration of the sources mapped to a synapse row
class SynapseRowSource
{
public:
	typedef HMF::HICANN::DriverDecoder Address;
#if !defined(PYPLUSPLUS)
	typedef Projection::synapse_type SynapseType;
#else
	typedef std::string SynapseType;
#endif

	typedef std::array< SynapseSource, HMF::Coordinate::NeuronOnHICANN::x_type::end > SynapseMapping;

	SynapseRowSource(SynapseType const& type);

	/// 2bit MSB of L1 Addresses forwarded on this line.
	Address const& prefix(size_t ii) const;
#if !defined(PYPLUSPLUS)
	Address&       prefix(size_t ii);
#endif

	SynapseMapping const& synapses() const;
#if !defined(PYPLUSPLUS)
	SynapseMapping &      synapses();
#endif

	/// SynapseType, either excitatory or inhibitory
	SynapseType const& type() const;

	// TODO: include synapse mapping in operators.
	bool operator== (SynapseRowSource const& rhs) const;
	bool operator!= (SynapseRowSource const& rhs) const;

	// for serialization only
	SynapseRowSource();

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);

	// TODO: place everything here which should later be set individually at the
	// SynapseDriver for a given route, e.g.: STP

	// the corrsponding projection is only needed, when weights are supposed to
	// be set externally. Rather than during the SynapseRow assignment.
	//HardwareProjection mProjection;

	/// 2bit MSB of L1 Addresses forwarded on this line.
	std::array<Address,2> mPrefix;

	/// SynapseType, either excitatory or inhibitory
	/// TODO: change to left/right
	SynapseType mType;

	SynapseMapping mSynapses; /// holds the biological synapses mapped to hw synapses
};


/// Routing configuration for a VLine
/// holds the configuration for all connected synapse drivers,
/// which are connected via the primary driver.
/// For all of these drivers, the row configuration is also stored
class DriverResult
{
public:
	typedef HMF::Coordinate::SynapseDriverOnHICANN SynapseDriverOnHICANN;
	typedef HMF::Coordinate::VLineOnHICANN VLineOnHICANN;

	typedef std::map<
			SynapseDriverOnHICANN,
			std::vector<SynapseDriverOnHICANN>
		> Drivers; /// mapping of primaray Synapse Driver to adjacent Synapse Drivers

	typedef std::map<
			HMF::Coordinate::SynapseRowOnHICANN,
			SynapseRowSource
		> Rows;

	DriverResult(VLineOnHICANN const& vline);

	/// Vertical line associated with this SynapseRow, either via corresponding
	/// or adjacent synapse drivers.
	VLineOnHICANN const& line() const;

	/// returns true, if `line()` corresponds to `VLineOnHICANN` from adjacent
	/// HICANN.
	bool from_adjacent() const;

	/// a mapping of SynapseRows to sources
#if !defined(PYPLUSPLUS)
	Rows&       rows();
#endif
	Rows const& rows() const;

	/// a mapping of primary SynapseDriver to adjacent SynapseDrivers
#if !defined(PYPLUSPLUS)
	Drivers&       drivers();
#endif
	Drivers const& drivers() const;

	/// check whether the VLine is connected to maximally 2 primary drivers
	/// TODO: why 2?
	void check() const;

	/// ugly hack, in due to crapy PY++ bindings
	std::vector<HMF::Coordinate::SynapseDriverOnHICANN> const&
	getDrivers(HMF::Coordinate::SynapseDriverOnHICANN const& drv) const
	{
		Drivers::const_iterator it = mDrivers.find(drv);
		if (it==mDrivers.end()) {
			throw std::runtime_error("out of range");
		}
		return it->second;
	}

private:
#if !defined(PYPLUSPLUS)
	/// serialization only
	DriverResult() = default;
#endif

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);

	/// corresponding VerticalLine, from where primary drivers receive their
	/// inputs.
	VLineOnHICANN mLine;

	/// a mapping of primary SynapseDriver to their adjacent Drivers driven by
	/// this primary.
	Drivers mDrivers;

	/// a mapping of row properties to rows.
	Rows mRows;
};



// implementations

template<typename Archiver>
void SynapseSource::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("view", mProjView)
	   & make_nvp("src", mSrc)
	   & make_nvp("tgt", mTgt);
}

template<typename Archiver>
void SynapseRowSource::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("prefix", mPrefix)
	   & make_nvp("type", mType)
	   & make_nvp("synapses", mSynapses);
}

template<typename Archiver>
void DriverResult::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("vline", mLine)
	   & make_nvp("drivers", mDrivers)
	   & make_nvp("rows", mRows);
}

} // namespace routing
} // namespace marocco
