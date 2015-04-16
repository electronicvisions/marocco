#pragma once
#include <unordered_map>
#include <array>
#include <boost/optional.hpp>

#include "HMF/NeuronCollection.h"

#include "marocco/config.h"
#include "marocco/Logger.h"
#include "marocco/routing/Result.h"
#include "marocco/placement/Result.h"
#include "marocco/parameter/detail.h"

namespace marocco {
namespace parameter {

/// This class is responsible for configuring the analog output. It further
/// keeps track of so far configured analog outputs
struct AnalogVisitor
{
	typedef void return_type;
	typedef size_t size_type;
	typedef chip_type<hardware_system_t>::type chip_t;
	typedef HMF::NeuronCollection calib_t;

	template <CellType N>
		using cell_t = TypedCellParameterVector<N>;


	void set_analog(
		chip_t& chip,
		calib_t const& calib,
		HMF::Coordinate::AnalogOnHICANN const& aout,
		HMF::Coordinate::NeuronOnHICANN const& nrn) const;


	// visitor for neurons which support Voltage Recording
	template <CellType N>
	typename std::enable_if<detail::has_record_v<cell_t<N>>::value, return_type>::type
	operator() (
		cell_t<N> const& v,
		calib_t const& calib,
		size_t const neuron_bio_id,
		HMF::Coordinate::NeuronOnHICANN const& nrn,
		chip_t& chip);


	// visitor for neurons which dno not support Voltage Recording
	template <CellType N>
	typename std::enable_if<!detail::has_record_v<cell_t<N>>::value, return_type>::type
	operator() (
		cell_t<N> const& v,
		calib_t const& calib,
		size_t const neuron_bio_id,
		HMF::Coordinate::NeuronOnHICANN const& nrn,
		chip_t& chip);

	size_t countActiveAouts() const
	{
		size_t cnt = 0;
		for (auto const& nrn : mMapping) {
			if (nrn) {
				++cnt;
			}
		}
		return cnt;
	}

private:
	std::array<boost::optional<HMF::Coordinate::NeuronOnHICANN>, 2> mMapping;
};


void transform_analog_outputs(
	AnalogVisitor::calib_t const& calib,
	Population const& pop,
	size_t const neuron_bio_id,
	HMF::Coordinate::NeuronOnHICANN const& nrn,
	AnalogVisitor& visitor,
	chip_type<hardware_system_t>::type& chip);

} // namespace parameter
} // namespace marocco
