#include "pymarocco/Routing.h"

#include "hal/HICANN/Crossbar.h"
#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

namespace pymarocco {

namespace {

Routing::switches_t initSwitches()
{
	using HMF::HICANN::Crossbar;

	Routing::switches_t sw;
	for (auto yy : iter_all<HLineOnHICANN>()) {
		for (auto xx : iter_all<VLineOnHICANN>()) {
			sw[xx][yy] = Crossbar::exists(xx, yy);
		}
	}
	return sw;
}

} // namespace

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
	_is_default(true)
{}

bool Routing::is_default() const
{
	return _is_default
		&& horizontal_line_swap == 2
		&& vertical_line_swap == 2;
}

bool Routing::cb_get(VLineOnHICANN const& x, HLineOnHICANN const& y) const
{
	return crossbar[x][y];
}

void Routing::cb_set(VLineOnHICANN const& x, HLineOnHICANN const& y, bool b)
{
	_is_default = false;
	crossbar[x][y] = b;
}

void Routing::cb_clear()
{
	_is_default = false;
	for (auto yy : iter_all<HLineOnHICANN>()) {
		for (auto xx : iter_all<VLineOnHICANN>()) {
			crossbar[xx][yy] = 0;
		}
	}
}

void Routing::cb_reset()
{
	_is_default = true;
	crossbar = initSwitches();
}

} // pymarocco
