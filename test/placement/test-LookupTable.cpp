#include <sstream>

#include "test/common.h"
#include "marocco/placement/LookupTable.h"

using namespace std;
using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

TEST(LookupTable, Serialization)
{
	using boost::serialization::make_nvp;

	LookupTable lut, lut2;
	std::ostringstream oss;
	boost::archive::xml_oarchive oa(oss);
	oa << make_nvp("myLut", lut);

	// FIXME: modify lut to be non-default constructed

	std::istringstream iss(oss.str());
	boost::archive::xml_iarchive ia(iss);
	ia >> make_nvp("myLut", lut2);

	// FIXME: compare lut and lut2
}

} // placement
} // marocco
