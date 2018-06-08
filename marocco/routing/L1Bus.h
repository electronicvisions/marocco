#pragma once

#include <boost/serialization/nvp.hpp>
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HMFUtil.h"

namespace marocco {
namespace routing {

struct L1Bus
{
	struct WrongBusType {};

	enum Direction {
		Horizontal,
		Vertical
	};

	typedef int busid_t;
	typedef Direction direction_t;

	L1Bus(direction_t dir, busid_t id, HMF::Coordinate::HICANNGlobal hicann);
	L1Bus(); // required for property map only

	direction_t getDirection() const;
	busid_t getBusId() const;

	HMF::Coordinate::VLineOnHICANN toVLine() const;
	HMF::Coordinate::HLineOnHICANN toHLine() const;

	HMF::Coordinate::HICANNGlobal hicann() const;

	HMF::Coordinate::Side side() const;

	bool operator== (L1Bus const& rhs) const;
	bool operator!= (L1Bus const& rhs) const;

private:
	/// This L1Bus representation either corresponds to a VLineOnHICANN or
	/// HLineOnHICANN depending on the direction.
	direction_t mDirection; // horizontal or vertial

	/// can either be bus id of VLine or HLineOnHICANN depending on the
	/// direction.
	busid_t mBusId;

	/// we also need the hicann address to have a global bus id.
	HMF::Coordinate::HICANNGlobal mHICANN;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/)
	{
		ar & boost::serialization::make_nvp("dir", mDirection)
		   & boost::serialization::make_nvp("bus_id", mBusId)
		   & boost::serialization::make_nvp("HICANN", mHICANN);
	}
};

} // namespace routing
} // namespace marocco
