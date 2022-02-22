#pragma once

#include <boost/serialization/export.hpp>
// GCCXML has problems with atomics -> removed before boost shared_prt is included
#ifdef PYPLUSPLUS
#undef __ATOMIC_RELAXED
#undef __ATOMIC_ACQUIRE
#undef __ATOMIC_RELEASE
#endif // PYPLUSPLUS
#include <boost/shared_ptr.hpp>

#include "sthal/Wafer.h"
#include "euter/metadata.h"
#include "pywrap/compat/macros.hpp"

#include "marocco/results/Marocco.h"

#include "marocco/placement/algorithms/ClusterByNeuronConnectivity.h"
#include "marocco/placement/algorithms/ClusterByPopulationConnectivity.h"
#include "marocco/placement/algorithms/PlacePopulationsBase.h"
#include "marocco/placement/algorithms/byNeuronBlockEnumAndPopulationID.h"
#include "marocco/placement/algorithms/byNeuronBlockEnumAndPopulationIDasc.h"
#include "marocco/placement/algorithms/bySmallerNeuronBlockAndPopulationID.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace pymarocco {
namespace runtime {

class Runtime : public euter::DerivedMetaData<Runtime>
{
public:
	std::string name() const PYPP_OVERRIDE;

	static boost::shared_ptr<Runtime> create();

	static boost::shared_ptr<Runtime> create(halco::hicann::v2::Wafer const& wafer);

	boost::shared_ptr<sthal::Wafer> wafer();
	boost::shared_ptr<marocco::results::Marocco> results();

	void clear_results();

private:
	Runtime();
	Runtime(halco::hicann::v2::Wafer const& wafer);

	boost::shared_ptr<sthal::Wafer> m_wafer;
	boost::shared_ptr<marocco::results::Marocco> m_results;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // Runtime

} // namespace runtime
} // namespace pymarocco

BOOST_CLASS_EXPORT_KEY(::pymarocco::runtime::Runtime)
