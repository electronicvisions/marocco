#include "marocco/placement/DescendingPopulationOrdering.h"

namespace marocco {
namespace placement {

bool DescendingPopulationOrdering::operator() (type const& a, type const& b) const {
	if (a.size() != b.size()) {
		return a.size() > b.size();
	} else if (a.population() != b.population()) {
		return a.population() < b.population();
	} else {
		return a.offset() < b.offset();
	}
}

} // namespace placement
} // namespace marocco
