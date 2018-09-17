#pragma once

#include <boost/serialization/export.hpp>
#include <boost/shared_ptr.hpp>

#include "sthal/Wafer.h"
#include "euter/metadata.h"
#include "pywrap/compat/macros.hpp"

#include "marocco/results/Marocco.h"

#include "marocco/placement/algorithms/PlacePopulationsBase.h"
#include "marocco/placement/algorithms/byNeuronBlockEnumAndPopulationID.h"
#include "marocco/placement/algorithms/bySmallerNeuronBlockAndPopulationID.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace pymarocco {
namespace runtime {

class Runtime : public DerivedMetaData<Runtime>
{
public:
	std::string name() const PYPP_OVERRIDE;

	static boost::shared_ptr<Runtime> create();

	static boost::shared_ptr<Runtime> create(HMF::Coordinate::Wafer const& wafer);

	boost::shared_ptr<sthal::Wafer> wafer();
	boost::shared_ptr<marocco::results::Marocco> results();

	void clear_results();

private:
	Runtime();
	Runtime(HMF::Coordinate::Wafer const& wafer);

	boost::shared_ptr<sthal::Wafer> m_wafer;
	boost::shared_ptr<marocco::results::Marocco> m_results;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // Runtime

} // namespace runtime
} // namespace pymarocco

BOOST_CLASS_EXPORT_KEY(::pymarocco::runtime::Runtime)
