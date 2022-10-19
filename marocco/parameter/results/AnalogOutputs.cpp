#include "marocco/parameter/results/AnalogOutputs.h"

#include <boost/serialization/nvp.hpp>

#include "halco/common/iter_all.h"

#include "marocco/util/iterable.h"

using namespace halco::hicann::v2;
using namespace halco::common;

using boost::multi_index::get;

namespace marocco {
namespace parameter {
namespace results {

AnalogOutputs::item_type::item_type()
{
}

AnalogOutputs::item_type::item_type(
    LogicalNeuron const& logical_neuron,
    analog_output_type const& analog_output,
    denmem_type const& recorded_denmem) :
    m_logical_neuron(logical_neuron),
    m_analog_output(analog_output),
    m_hicann(logical_neuron.front().toHICANNOnWafer()),
    m_reticle(m_hicann.toDNCOnWafer()),
    m_recorded_denmem(recorded_denmem)
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

auto AnalogOutputs::item_type::recorded_denmem() const -> denmem_type const&
{
	return m_recorded_denmem;
}

auto AnalogOutputs::record(LogicalNeuron const& logical_neuron) -> item_type const&
{
	if (logical_neuron.is_external()) {
		throw std::invalid_argument("external neurons cannot be recorded");
	}

	auto const reticle = logical_neuron.front().toHICANNOnWafer().toDNCOnWafer();

	// Find first free analog output and choose correct denmem
	for (auto const aout : iter_all<AnalogOnHICANN>()) {
		bool adc_free = true;
		auto const adc_group = reticle.toADCGroupOnWafer();
		for (auto const test_reticle : iter_all<DNCOnWafer>()) {
			// check if aout is already used on this ADCGroup
			if (test_reticle.toADCGroupOnWafer() == adc_group) {
				auto it = m_container.find(boost::make_tuple(test_reticle, aout));
				if (it != m_container.end()) {
					adc_free = false;

					auto const& other_neuron = it->logical_neuron();
					if (other_neuron == logical_neuron) {
						return *it;
					}
					if (other_neuron.shares_denmems_with(logical_neuron)) {
						throw ResourceInUseError("saw logical neurons with conflicting denmems");
					}

					break;
				}
			}
		}
		if (adc_free) {
			// Iterate over all denmems of a logical neuron and check if it is connected to a
			// different multiplexer line compared to the other recorded denmems.
			auto const hicann = logical_neuron.front().toHICANNOnWafer();
			for (auto const& recorded_denmem : logical_neuron) {
				auto multiplexer_line = recorded_denmem.toNeuronOnQuad();
				bool success = true;
				for (auto const& item : m_container) {
					auto const& other_recorded_denmem = item.recorded_denmem();
					auto const other_multiplexer_line = other_recorded_denmem.toNeuronOnQuad();
					auto const& other_hicann = item.hicann();
					if (hicann == other_hicann && multiplexer_line == other_multiplexer_line) {
						// Line already in use
						success = false;
						break;
					}
				}
				if (success) {
					auto res = m_container.insert(item_type(logical_neuron, aout, recorded_denmem));
					assert(res.second);
					return *res.first;
				}
			}
			// No free mulitplexer line found
			throw std::runtime_error("no free multiplexer line on HICANN");
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
void AnalogOutputs::item_type::serialize(Archiver& ar, const unsigned int version)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("logical_neuron", m_logical_neuron)
	   & make_nvp("analog_output", m_analog_output)
	   & make_nvp("hicann", m_hicann)
	   & make_nvp("reticle", m_reticle);
	if (version > 0) {
		ar & make_nvp("recorded_denmem", m_recorded_denmem);
	}
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
BOOST_CLASS_VERSION(::marocco::parameter::results::AnalogOutputs::item_type, 1)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::parameter::results::AnalogOutputs)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::parameter::results::AnalogOutputs::item_type)
