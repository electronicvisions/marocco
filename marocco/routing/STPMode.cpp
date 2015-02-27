#include "marocco/routing/STPMode.h"
#include "euter/synapses.h"
#include <boost/pointer_cast.hpp>

namespace marocco {
namespace routing {

std::ostream& operator<<(std::ostream& os, STPMode m)
{
	switch (m) {
		case STPMode::off:
			os << "off";
			break;
		case STPMode::depression:
			os << "depression";
			break;
		case STPMode::facilitation:
			os << "facilitation";
			break;
		default:
			throw std::runtime_error("unsupported switch case");
	}
	return os;
}

STPMode toSTPMode(boost::shared_ptr<const SynapseDynamics> const& dynamics)
{
	STPMode mode(STPMode::off);

	if (dynamics) {
		auto tso = boost::dynamic_pointer_cast<const TsodyksMarkramMechanism>(dynamics->fast());

		if (tso) {
			double tau_rec = tso->get("tau_rec");
			double tau_facil = tso->get("tau_facil");
			if (tau_rec > 0. && tau_facil == 0.) {
				mode = STPMode::depression;
			} else if (tau_rec == 0. && tau_facil > 0.) {
				mode = STPMode::facilitation;
			} else {
				throw std::runtime_error(
					"Unsupported parameters for TsodyksMarkramMechanism: "
					"only one of 'tau_rec' and 'tau_facil' "
					"can be non-zero on the HMF");
			}
		}
	}
	return mode;
}

} // namespace routing
} // namespace marocco
