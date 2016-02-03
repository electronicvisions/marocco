#include "marocco/placement/LookupTableData.h"

#include <ostream>

namespace marocco {
namespace placement {


bool hw_id::operator==(hw_id const &k) const
{
	return hicann == k.hicann && neuron_block == k.neuron_block && addr == k.addr;
}

bool hw_id::operator!=(hw_id const &k) const
{
	return !(*this == k);
}

std::ostream& hw_id::operator<< (std::ostream& os) const
{
	return os << hicann << " " << neuron_block << " " << addr;
}

bool bio_id::operator==(bio_id const &v) const
{
	return pop == v.pop && neuron == v.neuron;
}

bool bio_id::operator!=(bio_id const &v) const
{
	return !(*this == v);
}

std::ostream& bio_id::operator<< (std::ostream& os) const
{
	return os << pop << " " << neuron;
}

} // namespace placement
} // namespace marocco


namespace std {
size_t hash<marocco::placement::hw_id>::operator()(marocco::placement::hw_id const & t) const
{
	size_t hash = hash_value(t.hicann);
	boost::hash_combine(hash, hash_value(t.neuron_block));
	boost::hash_combine(hash, hash_value(t.addr));
	return hash;
}

size_t hash<marocco::placement::bio_id>::operator()(marocco::placement::bio_id const & t) const
{
	size_t hash = hash_value(t.pop);
	boost::hash_combine(hash, hash_value(t.neuron));
	return hash;
}
} // namespace std
