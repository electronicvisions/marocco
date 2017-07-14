#include "marocco/routing/Configuration.h"

#include <sstream>
#include <boost/variant/static_visitor.hpp>

using marocco::L1Route;
using namespace HMF::Coordinate;

class ConfigureL1RouteVisitor : public boost::static_visitor<>
{
	sthal::Wafer& m_hardware;
	HICANNOnWafer m_current_hicann;
	bool m_enable_test_data_output;

	sthal::HICANN& chip()
	{
		return m_hardware[m_current_hicann];
	}

	sthal::HICANN& chip(HICANNOnWafer const& hicann)
	{
		return m_hardware[hicann];
	}

public:
	ConfigureL1RouteVisitor(sthal::Wafer& hw, HICANNOnWafer hicann)
		: m_hardware(hw),
		  m_current_hicann(std::move(hicann)),
		  m_enable_test_data_output(false)
	{
	}

	void apply(L1Route::iterator it, L1Route::iterator const end)
	{
		auto next = std::next(it);
		for (; next != end; ++it, ++next) {
			boost::apply_visitor(*this, *it, *next);
		}
	}

	void apply(L1Route::segment_type const& current, L1Route::segment_type const& next)
	{
		boost::apply_visitor(*this, current, next);
	}

	//  ——— Crossbars ——————————————————————————————————————————————————————————

	void operator()(VLineOnHICANN const& current, HLineOnHICANN const& next)
	{
		chip().crossbar_switches.set(current, next, true);

		if (m_enable_test_data_output) {
			m_enable_test_data_output = false;
			auto repeater = current.toVRepeaterOnHICANN();
			auto direction = (repeater.toSideVertical() == top) ? bottom : top;
			auto& repeater_config = chip(m_current_hicann).repeater[repeater];
			repeater_config.setOutput(direction);
		}
	}

	void operator()(HLineOnHICANN const& current, VLineOnHICANN const& next)
	{
		chip().crossbar_switches.set(next, current, true);

		if (m_enable_test_data_output) {
			m_enable_test_data_output = false;
			auto repeater = current.toHRepeaterOnHICANN();
			auto direction = (repeater.toSideHorizontal() == left) ? right : left;
			auto& repeater_config = chip(m_current_hicann).repeater[repeater];
			repeater_config.setOutput(direction);
		}
	}

	//  ——— Sending Repeaters ——————————————————————————————————————————————————

	void operator()(DNCMergerOnHICANN const& current, HLineOnHICANN const& /*next*/)
	{
		chip().repeater[current.toSendingRepeaterOnHICANN().toHRepeaterOnHICANN()].setOutput(
		    right, true);
	}

	void operator()(DNCMergerOnHICANN const& current, HICANNOnWafer const& next)
	{
		chip().repeater[current.toSendingRepeaterOnHICANN().toHRepeaterOnHICANN()].setOutput(
		    left, true);

		m_current_hicann = next;
	}

	//  ——— Test ports —————————————————————————————————————————————————————————

	void operator()(RepeaterBlockOnHICANN const& /*current*/, HLineOnHICANN const& /*next*/)
	{
		m_enable_test_data_output = true;
	}

	void operator()(RepeaterBlockOnHICANN const& /*current*/, VLineOnHICANN const& /*next*/)
	{
		m_enable_test_data_output = true;
	}

	//  ——— Repeaters ——————————————————————————————————————————————————————————

	void operator()(HLineOnHICANN const& current, HICANNOnWafer const& next)
	{
		auto hicann = m_current_hicann;
		auto repeater = current.toHRepeaterOnHICANN();
		auto direction = m_current_hicann.x() < next.x() ? right : left;

		// The correct repeater for this direction may be on the adjacent HICANN.
		if (repeater.toSideHorizontal() != direction) {
			hicann = next;
			repeater = (direction == right ? current.east() : current.west()).toHRepeaterOnHICANN();
		}

		chip(hicann).repeater[repeater].setForwarding(direction);

		// Enable test output to current bus
		if (m_enable_test_data_output) {
			m_enable_test_data_output = false;
			chip(m_current_hicann).repeater[current.toHRepeaterOnHICANN()].setOutput(direction);
		}

		m_current_hicann = next;
	}

	void operator()(VLineOnHICANN const& current, HICANNOnWafer const& next)
	{
		auto hicann = m_current_hicann;
		auto repeater = current.toVRepeaterOnHICANN();
		auto direction = m_current_hicann.y() < next.y() ? bottom : top;

		// The correct repeater for this direction may be on the adjacent HICANN.
		if (repeater.toSideVertical() != direction) {
			hicann = next;
			repeater = (direction == top ? current.north() : current.south()).toVRepeaterOnHICANN();
		}

		chip(hicann).repeater[repeater].setForwarding(direction);

		// Enable test output to current bus
		if (m_enable_test_data_output) {
			m_enable_test_data_output = false;
			chip(m_current_hicann).repeater[current.toVRepeaterOnHICANN()].setOutput(direction);
		}

		m_current_hicann = next;
	}

	//  ——— No Operation ———————————————————————————————————————————————————————

	template <typename T>
	void operator()(HICANNOnWafer const& /*current*/, T const& /*next*/)
	{
	}

	//  ——— Not Implemented ————————————————————————————————————————————————————

	template <typename T, typename V>
	void operator()(T const& current, V const& next)
	{
		std::ostringstream err;
		err << "L1 configuration not implemented " << current << " -> " << next;
		throw std::runtime_error(err.str());
	}
};

namespace marocco {
namespace routing {

void configure(sthal::Wafer& hw, L1Route const& route)
{
	ConfigureL1RouteVisitor visitor(hw, route.source_hicann());
	visitor.apply(route.begin(), route.end());
}

void configure(sthal::Wafer& hw, L1RouteTree const& tree, ConfigureL1RouteVisitor& visitor)
{
	auto const& route = tree.head();
	visitor.apply(route.begin(), route.end());
	for (L1RouteTree const& tail : tree.tails()) {
		if (tail.empty()) {
			continue;
		}

		auto copy = visitor;
		auto it = tail.head().begin();
		if (route.target_hicann() == tail.head().source_hicann()) {
			++it;
		}
		copy.apply(route.back(), *it);
		configure(hw, tail, copy);
	}
}

void configure(sthal::Wafer& hw, L1RouteTree const& tree)
{
	ConfigureL1RouteVisitor visitor(hw, tree.head().source_hicann());
	configure(hw, tree, visitor);
}

} // namespace routing
} // namespace marocco
