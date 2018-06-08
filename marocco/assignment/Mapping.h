#pragma once

#include <vector>
#include <boost/make_shared.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "marocco/assignment/Hardware.h"

namespace marocco {
namespace assignment {

/**
 * Lightweight wrapper around a list of assignments. Distributed property maps
 * return entries by value. Therefore, it sensible not to copy the vector all
 * the time.
 **/
struct Mapping
{
	typedef Hardware assignment_type;

	Mapping() :
		mAssignment(boost::make_shared< std::vector<assignment_type> >())
	{}

	std::vector<assignment_type>&       assignment()       { return *mAssignment; }
	std::vector<assignment_type> const& assignment() const { return *mAssignment; }

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/) {
		using boost::serialization::make_nvp;
		ar & make_nvp("assignment", mAssignment);
	}

	boost::shared_ptr< std::vector<assignment_type> > mAssignment;
};

} // namespace assignment
} // namespace marocco
