#pragma once

#include <algorithm>
#include <type_traits>
#include <stdexcept>
#include <boost/shared_ptr.hpp>

#include "marocco/util.h"
#include "marocco/Algorithm.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/Result.h"
#include "marocco/routing/SynapseLoss.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

/*! \class abstract placement interface
 */
class Routing :
	public Algorithm
{
public:
	virtual ~Routing() {}
	template<typename ... Args>
	Routing(Args&& ... args) :
		Algorithm(std::forward<Args>(args)...) {}

	// placement start interface
	virtual std::unique_ptr<result_type> run(result_type const&) = 0;

	virtual boost::shared_ptr<SynapseLoss> getSynapseLoss() const
	{
		return boost::shared_ptr<SynapseLoss>();
	}
};


class DefaultRouting :
	public Routing
{
public:
	template<typename ... Args>
	DefaultRouting(pymarocco::PyMarocco& pymarocco, Args&& ... args);

	virtual
	std::unique_ptr<typename Routing::result_type>
	run(result_type const& placement_result);

	boost::shared_ptr<SynapseLoss> getSynapseLoss() const;

private:
	pymarocco::PyMarocco& mPyMarocco;
	boost::shared_ptr<SynapseLoss> mSynapseLoss;
};

template<typename ... Args>
DefaultRouting::DefaultRouting(pymarocco::PyMarocco& pymarocco, Args&& ... args) :
	Routing(std::forward<Args>(args)...),
	mPyMarocco(pymarocco),
	mSynapseLoss()
{
	// get all process local output buffer a.k.a. terminals
	//auto output_buffer = get_by_trait<TerminalTrait>();

	// make sure to have apropriate ghost cells in the graph for each route
	//request_remote_populations(output_buffer, proj_map, rev_map, mGraph);
}

} // namespace routing
} // namespace marocco
