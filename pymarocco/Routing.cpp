#include "pymarocco/Routing.h"

using namespace HMF::Coordinate;

namespace pymarocco {

namespace {

// taken from HALBe
bool exists(VLineOnHICANN x, HLineOnHICANN y)
{
	size_t y_, x_mod = x%32;

	y_ = y - y%2;
	y_ /= 2;

	return x<128 ? (31-y_)==x_mod : y_==x_mod;
}

Routing::switches_t initSwitches()
{
	Routing::switches_t sw;
	for (size_t yy = 0; yy < HLineOnHICANN::end; ++yy) {
		for (size_t xx = 0; xx < VLineOnHICANN::end; ++xx) {
			sw.at(xx).at(yy) = exists(VLineOnHICANN(xx), HLineOnHICANN(yy));
		}
	}
	return sw;
}
} // anonymous


Routing::Routing() :
	shuffle_crossbar_switches(true),
	crossbar(initSwitches()),
	horizontal_line_swap(2),
	vertical_line_swap(2),
	syndriver_chain_length(56),
	max_distance_factor(2.0),
	weight_Vertical(5),
	weight_Horizontal(8),
	weight_SPL1(8),
	weight_StraightHorizontal(5),
	weight_StraightVertical(2),
	weight_CongestionFactor(0),
	_is_default(true),
	merger_tree_strategy(MergerTreeStrategy::minSPL1)
{}

bool Routing::is_default() const
{
	return _is_default
		&& horizontal_line_swap == 2
		&& vertical_line_swap == 2;
}

bool Routing::cb_get(VLineOnHICANN const& x, HLineOnHICANN const& y) const
{
	return crossbar.at(x).at(y);
}

void Routing::cb_set(VLineOnHICANN const& x, HLineOnHICANN const& y, bool b)
{
	_is_default = false;
	crossbar.at(x).at(y) = b;
}

void Routing::cb_clear()
{
	_is_default = false;
	for (size_t xx = 0; xx < VLineOnHICANN::end; ++xx) {
		for (size_t yy = 0; yy < HLineOnHICANN::end; ++yy) {
			crossbar.at(xx).at(yy) = 0;
		}
	}
}

void Routing::cb_reset()
{
	_is_default = true;
	crossbar = initSwitches();
}

} // pymarocco
