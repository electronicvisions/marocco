#pragma once

#include <vector>

#include "hal/Coordinate/Neuron.h"
#include "hal/Coordinate/Wafer.h"

namespace marocco {

class NodeOnFPGA : public HMF::Coordinate::detail::BaseType<NodeOnFPGA, size_t>
{
public:
	PYPP_CONSTEXPR explicit NodeOnFPGA(value_type val) : base_t(val)
	{
	}
	PYPP_CONSTEXPR explicit NodeOnFPGA() : base_t()
	{
	}
};

class LogicalNeuron
{
	typedef HMF::Coordinate::Wafer wafer_type;
	typedef HMF::Coordinate::NeuronGlobal denmem_type;
	typedef std::vector<denmem_type> container_type;
	typedef NodeOnFPGA external_node_type;

public:
	LogicalNeuron(wafer_type wafer, container_type const& denmems);
	LogicalNeuron(wafer_type wafer, external_node_type external_node);

	bool is_external() const;

	wafer_type const& wafer() const;
	container_type const& denmems() const;
	external_node_type const& external_node() const;

private:
	wafer_type m_wafer;
	container_type m_denmems;
	external_node_type m_external_node;
}; // LogicalNeuron

} // namespace marocco
