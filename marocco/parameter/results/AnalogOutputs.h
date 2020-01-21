#pragma once

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/external.h"
#include "halco/hicann/v2/hicann.h"
#include "pywrap/compat/macros.hpp"

#include "marocco/coordinates/LogicalNeuron.h"
#include "marocco/util.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace parameter {
namespace results {

/**
 * @brief Stores assignment of analog outputs to neurons.
 * Although there are two analog outputs per HICANN, those lines are shared by all HICANNs
 * of one reticle.  Thus only two neurons per reticle can be recorded.
 * Furthermore, recording of a denmem circuit is enabled via two multiplexers on each
 * HICANN (one for each analog output).  Each multiplexer can choose one of four lines and
 * for each line a single denmem can be connected.  See the following sketch (which should
 * only be seen a qualitative representation, the connectivity may not actually match the
 * actual hardware design).
 * \code
 *               .------------+-------+-------+--- - -
 *               |            |       |       |
 *               | .------+---(---+---(---+---(--- - -
 *         __    | |      |   |   |   |   |   |
 *        /  |-1-' |    +-o-+-o-+-o-+-o-+-o-+-o-+- - -
 *  ------|  |-2---'    | A | B | A | B | A | B |
 * 2x MUX |  |          +---+---+---+---+---+---+- - -
 *  ------|  |-3---.    | C | D | C | D | C | D |
 *        \__|-4-. |    +-o-+-o-+-o-+-o-+-o-+-o-+- - -
 *               | |      |   |   |   |   |   |
 *               | `------+---(---+---(---+---(--- - -
 *               |            |       |       |
 *               `------------+-------+-------+--- - -
 * \endcode
 * Note that each line connects to all denmems of a HICANN with the same NeuronOnQuad
 * coordinate.  Thus, we cannot simply always use the first denmem of a logical neuron to
 * record its membrane as that would e.g. lead to two denmems assigned to the same line
 * given two adjacent logical neurons of size 4.
 * To this effect we store the assignment of logical neurons to analog outputs here and
 * pick a sensible set of denmems during configuration.
 */
class AnalogOutputs
{
public:
	typedef halco::hicann::v2::AnalogOnHICANN analog_output_type;
	typedef halco::hicann::v2::DNCOnWafer reticle_type;
	typedef halco::hicann::v2::HICANNOnWafer hicann_type;

    class item_type {
    public:
		item_type(LogicalNeuron const& logical_neuron, analog_output_type const& analog_output);

		LogicalNeuron const& logical_neuron() const;
		analog_output_type const& analog_output() const;

	    hicann_type const& hicann() const;
		reticle_type const& reticle() const;

	private:
		LogicalNeuron m_logical_neuron;
		analog_output_type m_analog_output;
		hicann_type m_hicann;
		reticle_type m_reticle;

		friend class boost::serialization::access;
		item_type();
		template <typename Archiver>
		void serialize(Archiver& ar, const unsigned int /* version */);
	}; // item_type

	typedef boost::multi_index::multi_index_container<
	    item_type,
	    boost::multi_index::indexed_by<
	        boost::multi_index::hashed_unique<
	            boost::multi_index::composite_key<
	                item_type,
	                boost::multi_index::
	                    const_mem_fun<item_type, reticle_type const&, &item_type::reticle>,
	                boost::multi_index::
	                    const_mem_fun<item_type, analog_output_type const&, &item_type::analog_output> > >,
		    // We provide iteration over items ordered by HICANN to ease implementing the
		    // multiplexer constraint during configuration.
	        boost::multi_index::ordered_non_unique<
	            boost::multi_index::tag<hicann_type>,
		        boost::multi_index::const_mem_fun<item_type, hicann_type const&, &item_type::hicann> > > >
	    container_type;
	typedef container_type::index<hicann_type>::type by_hicann_type;
	typedef by_hicann_type::iterator iterator;
	typedef by_hicann_type::iterator const_iterator;

	/**
	 * @brief Request the membrane of the specified neuron to be recorded.
	 * @return Information about assigned analog output, if successful.
	 *         Potential previous assignments will also be returned.
	 * @throw ResourceExhaustedError If no further analog outputs are available.
	 */
	item_type const& record(LogicalNeuron const& logical_neuron);

	/**
	 * @brief Cancel a request for the membrane of the specified neuron to be recorded.
	 * @note This removes the request from the container, accessing eventual references
	 *       previously returned by #record() will lead to undefined behavior.
	 */
	bool unrecord(LogicalNeuron const& logical_neuron);

	bool empty() const;

	size_t size() const;

	iterator begin() const;

	iterator end() const;

private:
	container_type m_container;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // AnalogOutputs

} // namespace results
} // namespace parameter
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::parameter::results::AnalogOutputs)
BOOST_CLASS_EXPORT_KEY(::marocco::parameter::results::AnalogOutputs::item_type)
