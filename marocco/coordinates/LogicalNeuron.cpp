#include "marocco/coordinates/LogicalNeuron.h"

namespace marocco {

LogicalNeuron::LogicalNeuron(wafer_type wafer, container_type const& denmems)
	: m_wafer(std::move(wafer)), m_denmems(denmems)
{
	if (denmems.empty())
		throw std::runtime_error("LogicalNeuron must have denmems");
}

LogicalNeuron::LogicalNeuron(wafer_type wafer, external_node_type external_node)
	: m_wafer(std::move(wafer)), m_external_node(external_node)
{
}

bool LogicalNeuron::is_external() const
{
	return m_denmems.empty();
}

auto LogicalNeuron::wafer() const -> wafer_type const&
{
	return m_wafer;
}

auto LogicalNeuron::denmems() const -> container_type const&
{
	return m_denmems;
}

auto LogicalNeuron::external_node() const -> external_node_type const&
{
	if (!is_external()) {
		throw std::runtime_error("external_node() called on hardware neuron");
	}
	return m_external_node;
}

} // namespace marocco
