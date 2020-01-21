#include "pymarocco/runtime/Runtime.h"

#include <boost/make_shared.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/shared_ptr.hpp>

namespace pymarocco {
namespace runtime {

std::string Runtime::name() const
{
	return "marocco_runtime";
}

boost::shared_ptr<Runtime> Runtime::create()
{
	return {};
}

boost::shared_ptr<Runtime> Runtime::create(halco::hicann::v2::Wafer const& wafer)
{
#ifndef __ESTER_BREACHED__
	throw std::runtime_error("only supported in --without-ester mode");
#endif // !__ESTER_BREACHED__

	return boost::shared_ptr<Runtime>(new Runtime(wafer));
}

boost::shared_ptr<sthal::Wafer> Runtime::wafer()
{
	return m_wafer;
}

boost::shared_ptr<marocco::results::Marocco> Runtime::results()
{
	return m_results;
}

void Runtime::clear_results() {
	m_results.reset(new marocco::results::Marocco);
}

Runtime::Runtime()
{
}

Runtime::Runtime(halco::hicann::v2::Wafer const& wafer)
	: m_wafer(new sthal::Wafer(wafer)), m_results(new marocco::results::Marocco())
{
}

template <typename Archive>
void Runtime::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("wafer", m_wafer)
	   & make_nvp("results", m_results);
	// clang-format on
}

} // namespace runtime
} // namespace pymarocco

BOOST_CLASS_EXPORT_IMPLEMENT(::pymarocco::runtime::Runtime)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::pymarocco::runtime::Runtime)
