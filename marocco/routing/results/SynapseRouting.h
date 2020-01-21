#pragma once

// #include <map>
#ifndef PYPLUSPLUS
#include <unordered_map>
#endif // !PYPLUSPLUS
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/l1.h"
#include "halco/hicann/v2/synapse.h"
#include "pywrap/compat/macros.hpp"

#include "marocco/routing/results/ConnectedSynapseDrivers.h"
#include "marocco/routing/results/SynapseDriverConfiguration.h"
#include "marocco/routing/results/SynapseRowConfiguration.h"
#include "marocco/routing/results/Synapses.h"
#include "marocco/routing/results/SynapticInputs.h"
#include "marocco/util/iterable.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace results {

class SynapseRouting {
public:
	// TODO: This could have been a multimap but py++ will mark `std::pair<Key const, T>` with
	// `boost::noncopyable` because the `first` member cannot be copy-assigned (because of the
	// const).  This leads to `No to_python (by-value) converter found for C++ type: std::pair<...>`.
	// typedef std::multimap<halco::hicann::v2::VLineOnHICANN, ConnectedSynapseDrivers>
	// 	synapse_switches_type;
	class synapse_switches_item_type
	{
	public:
		typedef halco::hicann::v2::VLineOnHICANN source_type;

		synapse_switches_item_type(
			source_type const& source, ConnectedSynapseDrivers const& connected_drivers);
		source_type const& source() const;
		ConnectedSynapseDrivers const& connected_drivers() const;

	private:
		source_type m_source;
		ConnectedSynapseDrivers m_connected_drivers;

		friend class boost::serialization::access;
		synapse_switches_item_type();
		template <typename Archiver>
		void serialize(Archiver& ar, const unsigned int /* version */);
	}; // synapse_switches_item_type

	typedef boost::multi_index::multi_index_container<
		synapse_switches_item_type,
		boost::multi_index::indexed_by<boost::multi_index::hashed_non_unique<
			boost::multi_index::tag<synapse_switches_item_type::source_type>,
			boost::multi_index::const_mem_fun<synapse_switches_item_type,
			                                  synapse_switches_item_type::source_type const&,
			                                  &synapse_switches_item_type::source> > > >
		synapse_switches_type;

	class HICANN {
	public:
		bool has(halco::hicann::v2::SynapseDriverOnHICANN const& driver) const;

		bool has(halco::hicann::v2::SynapseRowOnHICANN const& row) const;

		SynapseDriverConfiguration& operator[](
			halco::hicann::v2::SynapseDriverOnHICANN const& driver);

		SynapseDriverConfiguration const& operator[](
			halco::hicann::v2::SynapseDriverOnHICANN const& driver) const;

		SynapseRowConfiguration& operator[](
			halco::hicann::v2::SynapseRowOnHICANN const& row);

		SynapseRowConfiguration const& operator[](
			halco::hicann::v2::SynapseRowOnHICANN const& row) const;

		/**
		 * @brief Add connection from vertical bus to synapse drivers.
		 * @param vline End of afferent route, may lie on adjacent HICANN.
		 */
		void add_synapse_switch(
			halco::hicann::v2::VLineOnHICANN const& vline,
			ConnectedSynapseDrivers const& drivers);

		iterable<synapse_switches_type::iterator> operator[](
		    halco::hicann::v2::VLineOnHICANN const& vline) const;

		iterable<synapse_switches_type::iterator> synapse_switches() const;

#ifndef PYPLUSPLUS
		SynapticInputs& synaptic_inputs();
#endif // !PYPLUSPLUS

		SynapticInputs const& synaptic_inputs() const;

	private:
#ifndef PYPLUSPLUS
		std::unordered_map<halco::hicann::v2::SynapseDriverOnHICANN, SynapseDriverConfiguration>
			m_synapse_driver_configurations;
		std::unordered_map<halco::hicann::v2::SynapseRowOnHICANN, SynapseRowConfiguration>
		    m_synapse_rows;
#endif // !PYPLUSPLUS
		synapse_switches_type m_synapse_switches;
		SynapticInputs m_synaptic_inputs;

		friend class boost::serialization::access;
		template <typename Archiver>
		void serialize(Archiver& ar, const unsigned int /* version */);
	}; // HICANN

	bool has(halco::hicann::v2::HICANNOnWafer const& hicann) const;

	HICANN& operator[](halco::hicann::v2::HICANNOnWafer const& hicann);

	HICANN const& operator[](halco::hicann::v2::HICANNOnWafer const& hicann) const;

#ifndef PYPLUSPLUS
	Synapses& synapses();
#endif // !PYPLUSPLUS

	Synapses const& synapses() const;

private:
#ifndef PYPLUSPLUS
	std::unordered_map<halco::hicann::v2::HICANNOnWafer, HICANN> m_hicanns;
#endif // !PYPLUSPLUS
	Synapses m_synapses;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // SynapseRouting

PYPP_INSTANTIATE(iterable<SynapseRouting::synapse_switches_type::iterator>)

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::SynapseRouting)
BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::SynapseRouting::HICANN)
