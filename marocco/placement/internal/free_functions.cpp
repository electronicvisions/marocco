#include "marocco/placement/internal/free_functions.h"

#include "marocco/Logger.h"

namespace marocco {
namespace placement {
namespace internal {

OnNeuronBlock& get_on_neuron_block_reference(
    Result::denmem_assignment_type& denmem_state, HMF::Coordinate::NeuronBlockOnWafer const& nb)
{
	auto const hicann = nb.toHICANNOnWafer();
	auto it = denmem_state.find(hicann);
	if (it == denmem_state.end()) {
		// HICANN is not available, this can only happen for manual placement requests because
		// automatic placement loops over available HICANNs when determining available neuron
		// blocks.
		//
		// the entry might be missing in case of missing redman data or non-wafer setups => we have
		// to check existance and throw an error if it is missing
		LOG4CXX_FATAL(
		    log4cxx::Logger::getLogger("marocco"),
		    "NeuronBlock "
		        << nb << " on " << hicann
		        << " is unavailable during manual population placement manually specified denmems "
		           "are not present or data is missing in the database");
		throw std::runtime_error(
		    "Neuron/NeuronBlock/HICANN unavailable during manual population placement");
	}
	return it->second[nb.toNeuronBlockOnHICANN()];
}

} // namespace internal
} // namespace placement
} // namespace marocco
