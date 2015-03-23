#include "marocco/placement/ReverseMapping.h"

#include <ostream>

#include "marocco/placement/Result.h"
#include "hal/Coordinate/iter_all.h"

namespace marocco {
namespace placement {

bool RevKey::operator== (RevKey const& k) const
{
	return hicann == k.hicann
		&& outb == k.outb
		&& addr == k.addr;
}

bool RevKey::operator!= (RevKey const& k) const
{
	return !(*this == k);
}

std::ostream& RevKey::operator<< (std::ostream& os) const
{
	return os << hicann << " " << outb << " " << addr;
}

bool RevVal::operator== (RevVal const& v) const
{
	return pop == v.pop
		&& neuron == v.neuron;
}

bool RevVal::operator!= (RevVal const& v) const
{
	return !(*this == v);
}

std::ostream& RevVal::operator<< (std::ostream& os) const
{
	return os << pop << " " << neuron;
}

ReverseMapping::ReverseMapping(Result const& result,
							   resource_manager_t const& mgr,
							   graph_t const& graph)
{
	using namespace HMF::Coordinate;
	auto const& popmap = boost::get(population_t(), graph);

	auto const& om = result.output_mapping;
	for (auto const& h : mgr.allocated())
	{
		for (auto const& outb : iter_all<OutputBufferOnHICANN>())
		{
			auto const& mappings = om.at(h).at(outb);
			for (assignment::AddressMapping const& am : mappings)
			{
				auto const& addresses = am.addresses();
				auto const& bio = am.bio();

				Population const& pop = *popmap[bio.population()];

				size_t neuron_index = bio.offset();
				size_t offset = 0;
				for (auto const& address : addresses) {

					MAROCCO_TRACE("RevVal: pop: " << pop.id() << " neuron: " << neuron_index << " offset: " << offset << ", RevKey: " << h << " " << outb << " " << address);

					key_type const key {h, outb, address};
					mMapping[key] = mapped_type {pop.id(), neuron_index + offset};
					++offset;
				}
			}
		}
	}
}

ReverseMapping::mapped_type&
ReverseMapping::operator[] (key_type const& key)
{
	return mMapping[key];
}

ReverseMapping::mapped_type&
ReverseMapping::at(key_type const& key)
{
	return mMapping.at(key);
}

ReverseMapping::mapped_type
const& ReverseMapping::at(key_type const& key) const
{
	return mMapping.at(key);
}

ReverseMapping::size_type ReverseMapping::size() const
{
	return mMapping.size();
}

bool ReverseMapping::empty() const
{
	return mMapping.empty();
}

} // namespace placement
} // namespace marocco

namespace std {

size_t hash<marocco::placement::RevKey>::operator()(type const & t) const
{
	size_t hash = hash_value(t.hicann);
	boost::hash_combine(hash, hash_value(t.outb));
	boost::hash_combine(hash, hash_value(t.addr));
	return hash;
}

} // namespace std
