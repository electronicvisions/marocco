#include "marocco/placement/MergerRouting.h"

#include <unordered_map>
#include <boost/optional.hpp>

#include "halco/common/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/placement/ConstrainMergers.h"
#include "marocco/placement/MergerTreeConfigurator.h"
#include "marocco/placement/MergerTreeGraph.h"
#include "marocco/placement/MergerTreeRouter.h"
#include "marocco/placement/parameters/MergerRouting.h"
#include "marocco/resource/Manager.h"
#include "marocco/util.h"
#include "marocco/util/chunked.h"

using namespace halco::hicann::v2;
using namespace halco::common;
using marocco::assignment::PopulationSlice;

namespace marocco {
namespace placement {

MergerRouting::MergerRouting(
    parameters::MergerRouting const& parameters,
    internal::Result::denmem_assignment_type const& denmem_assignment,
    MergerRoutingResult& result,
    resource::HICANNManager const& res_mgr,
    results::Placement const& placement,
    graph_t const& bio_graph,
    parameters::L1AddressAssignment const& address_parameters) :
    m_parameters(parameters),
    m_denmem_assignment(denmem_assignment),
    m_result(result),
    m_res_mgr(res_mgr),
    m_placement(placement),
    m_bio_graph(bio_graph),
    m_address_parameters(address_parameters)
{
}

void MergerRouting::run(MergerTreeGraph const& graph, halco::hicann::v2::HICANNOnWafer const& hicann)
{
	MAROCCO_TRACE("Running merger routing for " << hicann);

	auto& merger_mapping = m_result[hicann];

	for (auto const nb : iter_all<NeuronBlockOnHICANN>()) {
		merger_mapping[nb] = DNCMergerOnHICANN(nb);
	}

	switch(m_parameters.strategy()) {
		case parameters::MergerRouting::Strategy::minimize_number_of_sending_repeaters: {
			// MergerTreeRouting tries to merge adjacent neuron blocks such that the
			// overall use of SPL1 outputs is minimized.  Every unused SPL1 output can
			// then be used for external input.

			MergerTreeRouter router(graph, m_denmem_assignment, boost::none);
			router.run(hicann);

			// If there is no entry in the merger mapping result, the corresponding neuron
			// block has not been merged with any other blocks and thus a 1-to-1
			// connection should be possible (only adjacent blocks are merged!).
			for (auto const& item : router.result()) {
				merger_mapping[item.first] = item.second;
			}

			break;
		}
		case parameters::MergerRouting::Strategy::minimize_as_possible: {
			// MergerTreeRoutingConstrained tries to merge adjacent neuron blocks such that the
			// overall use of SPL1 outputs is minimized, but it does not merge more blocks than
			// the SynapseDriversChainLength is able to handle, based on calculations of the
			// SynapseDriverChainLengthPerSource class. Every unused SPL1 output can be used for
			// external input, as a background generator can be merged to it.

			ConstrainMergers constrainer(
			    m_res_mgr, m_placement, m_bio_graph, m_address_parameters.strategy());


			MergerTreeRouter router(
			    graph, m_denmem_assignment, boost::optional<ConstrainMergers>(constrainer));

			router.run(hicann);

			// If there is no entry in the merger mapping result, the corresponding neuron
			// block has not been merged with any other blocks and thus a 1-to-1
			// connection should be possible (only adjacent blocks are merged!).
			for (auto const& item : router.result()) {
				merger_mapping[item.first] = item.second;
			}

			break;
		}
		case parameters::MergerRouting::Strategy::one_to_one: {
			break;
		}
		default:
			throw std::runtime_error("unknown merger tree strategy");
	} // switch merger tree strategy
}

} // namespace placement
} // namespace marocco
