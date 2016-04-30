#include "marocco/results/Marocco.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace marocco {
namespace results {

Marocco Marocco::from_file(char const* filename)
{
	boost::filesystem::path path(filename);
	if (!boost::filesystem::exists(path)) {
		throw std::runtime_error("file not found");
	}
	boost::filesystem::ifstream stream(path);

	Marocco result;
	boost::archive::binary_iarchive{stream} >> result;
	return result;
}

void Marocco::save(char const* filename, bool overwrite) const
{
	boost::filesystem::path path(filename);
	if (boost::filesystem::exists(path) && !overwrite) {
		throw std::runtime_error("file already exists");
	}

	boost::filesystem::ofstream stream(path);
	boost::archive::binary_oarchive{stream} << *this;
}

template <typename Archiver>
void Marocco::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("placement", placement);
	// clang-format on
}

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::results::Marocco)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::results::Marocco)
