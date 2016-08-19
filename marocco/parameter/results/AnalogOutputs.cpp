#include "marocco/parameter/results/AnalogOutputs.h"

#include <boost/serialization/nvp.hpp>

#include "hal/Coordinate/iter_all.h"

#include "marocco/util/iterable.h"

using namespace HMF::Coordinate;
using boost::multi_index::get;

namespace marocco {
namespace parameter {
namespace results {

AnalogOutputs::item_type::item_type()
{
}

AnalogOutputs::item_type::item_type(
	LogicalNeuron const& logical_neuron, analog_output_type const& analog_output)
	: m_logical_neuron(logical_neuron),
	  m_analog_output(analog_output),
	  m_hicann(logical_neuron.front().toHICANNOnWafer()),
	  m_reticle(m_hicann.toDNCOnWafer())
{
}

auto AnalogOutputs::item_type::logical_neuron() const -> LogicalNeuron const&
{
	return m_logical_neuron;
}

auto AnalogOutputs::item_type::analog_output() const -> analog_output_type const&
{
	return m_analog_output;
}

auto AnalogOutputs::item_type::hicann() const -> hicann_type const&
{
	return m_hicann;
}

auto AnalogOutputs::item_type::reticle() const -> reticle_type const&
{
	return m_reticle;
}

auto AnalogOutputs::record(LogicalNeuron const& logical_neuron) -> item_type const&
{
	if (logical_neuron.is_external()) {
		throw std::invalid_argument("external neurons cannot be recorded");
	}

	auto const reticle = logical_neuron.front().toHICANNOnWafer().toDNCOnWafer();

	// Find first free analog output.
	for (auto const aout : iter_all<AnalogOnHICANN>()) {
		auto it = m_container.find(boost::make_tuple(reticle, aout));
		if (it == m_container.end()) {
			auto res = m_container.insert(item_type(logical_neuron, aout));
			assert(res.second);
			return *res.first;
		}

		auto const& other_neuron = it->logical_neuron();
		if (other_neuron == logical_neuron) {
			return *it;
		}

		if (other_neuron.shares_denmems_with(logical_neuron)) {
			throw ResourceInUseError("saw logical neurons with conflicting denmems");
		}
	}

	throw ResourceExhaustedError("no analog outputs left");
}

bool AnalogOutputs::unrecord(LogicalNeuron const& logical_neuron)
{
	if (logical_neuron.is_external()) {
		throw std::invalid_argument("external neurons cannot be recorded");
	}

	auto const hicann = logical_neuron.front().toHICANNOnWafer();

	iterator it, eit;
	std::tie(it, eit) = get<hicann_type>(m_container).equal_range(hicann);

	for (; it != eit; ++it) {
		if (it->logical_neuron() == logical_neuron) {
			get<hicann_type>(m_container).erase(it);
			return true;
		}
	}

	return false;
}

bool AnalogOutputs::empty() const
{
	return m_container.empty();
}

size_t AnalogOutputs::size() const
{
	return m_container.size();
}

auto AnalogOutputs::begin() const -> iterator {
	return get<hicann_type>(m_container).begin();
}

auto AnalogOutputs::end() const -> iterator {
	return get<hicann_type>(m_container).end();
}

template <typename Archiver>
void AnalogOutputs::item_type::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("logical_neuron", m_logical_neuron)
	   & make_nvp("analog_output", m_analog_output)
	   & make_nvp("hicann", m_hicann)
	   & make_nvp("reticle", m_reticle);
	// clang-format on
}

template <typename Archiver>
void AnalogOutputs::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("container", m_container);
	// clang-format on
}

} // namespace results
} // namespace parameter
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::parameter::results::AnalogOutputs)
BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::parameter::results::AnalogOutputs::item_type)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::parameter::results::AnalogOutputs)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::parameter::results::AnalogOutputs::item_type)
