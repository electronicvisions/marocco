#include "marocco/coordinates/LogicalNeuron.h"

#include <bitset>
#include <boost/bind.hpp>
#include <boost/functional/hash.hpp>
#include <boost/ref.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/utility.hpp>

#include "hal/Coordinate/geometry.h"
#include "hal/Coordinate/iter_all.h"
#include "marocco/coordinates/printers.h"

using namespace HMF::Coordinate;

namespace marocco {

void check_neuron_size(size_t size)
{
	if (size == 0 || ((size % 2) != 0) ||
	    (size > HMF::Coordinate::NeuronOnNeuronBlock::enum_type::size)) {
		throw std::invalid_argument(
		    "marocco requires multiple-of-two neuron size, must fit on neuron block");
	}
}


LogicalNeuron::iterator::iterator(
	neuron_block_type const& block, underlying_iterator const& chunk_iterator)
	: m_block(block), m_chunk_iterator(chunk_iterator), m_offset(0u)
{
}

bool LogicalNeuron::iterator::equal(iterator const& other) const
{
	return m_chunk_iterator == other.m_chunk_iterator && m_offset == other.m_offset;
}

void LogicalNeuron::iterator::increment()
{
	// Dereferencing is okay because calling increment on an end iterator has undefined
	// behavior anyways.
	auto const& size = m_chunk_iterator->second;

	if (++m_offset >= size) {
		m_offset = 0;
		++m_chunk_iterator;
	}
}

auto LogicalNeuron::iterator::dereference() const -> neuron_type
{
	auto const& chunk_neuron = m_chunk_iterator->first;
	return neuron_on_block_type(X(chunk_neuron.x() + m_offset), Y(chunk_neuron.y()))
		.toNeuronOnWafer(m_block);
}

LogicalNeuron::Builder::Builder(neuron_block_type const& block) : m_block(block), m_chunks()
{
}

auto LogicalNeuron::Builder::add(neuron_on_block_type const& offset, size_t const size) -> Builder&
{
	push_back(std::make_pair(offset, size));
	return *this;
}

void LogicalNeuron::Builder::push_back(chunk_type const& chunk)
{
	if (chunk.second == 0u) {
		throw std::invalid_argument("neuron chunk has to be composed of at least one denmem");
	}
	if (!m_chunks.insert(chunk).second) {
		throw std::invalid_argument("overlapping chunks in neuron");
	}
}

auto LogicalNeuron::Builder::squash() -> Builder&
{
	for (auto it = m_chunks.begin(), eit = m_chunks.end(); it != eit; ++it) {
		chunk_type const& chunk = *it;
		// Check if the next chunk can be merged into the current chunk.
		for (auto next = std::next(it); next != eit && next->first.y() == chunk.first.y() &&
		                                next->first.x() == chunk.first.x() + chunk.second;
		     next = std::next(it)) {
			// We do not change the value used for sorting, so this is safe.
			const_cast<chunk_type&>(chunk).second += next->second;
			m_chunks.erase(next);
		}
	}
	return *this;
}

LogicalNeuron LogicalNeuron::Builder::done()
{
	if (m_chunks.empty()) {
		throw std::invalid_argument("logical neuron needs at least one chunk");
	}
	LogicalNeuron ret(m_block, std::move(m_chunks));
	m_chunks.clear();
	return ret;
}

auto LogicalNeuron::on(neuron_block_type const& block) -> Builder
{
	return Builder(block);
}

LogicalNeuron LogicalNeuron::rectangular(neuron_type const& topleft, size_t const size)
{
	if (topleft.y() != 0) {
		throw std::invalid_argument("rectangular neuron has to start in top row");
	}
	check_neuron_size(size);

	auto const column = topleft.toNeuronOnNeuronBlock().x();
	return on(topleft.toNeuronBlockOnWafer())
		.add(NeuronOnNeuronBlock(column, Y(0)), size - size / 2)
		.add(NeuronOnNeuronBlock(column, Y(1)), size / 2)
		.done();
}

LogicalNeuron LogicalNeuron::external(
    external_identifier_type const external_identifier, size_t const index)
{
	return LogicalNeuron(external_identifier, index);
}

LogicalNeuron::LogicalNeuron() : m_external_identifier(0u), m_external_index(0u)
{
}

LogicalNeuron::LogicalNeuron(external_identifier_type const external_identifier, size_t const index)
	: m_external_identifier(external_identifier), m_external_index(index)
{
}

LogicalNeuron::LogicalNeuron(neuron_block_type const& block, container_type&& chunks)
	: m_external_identifier(0u), m_size(0u), m_block(block), m_chunks(std::move(chunks))
{
	if (m_chunks.empty()) {
		throw std::invalid_argument("neuron has to have at least one chunk");
	}

	typedef std::bitset<neuron_on_block_type::x_type::size> bitset_type;
	// Track position of denmems in last row.  Set to true initially because denmems are
	// allowed to occur at every position in the first row.
	bitset_type last_row;
	last_row.set();

	for (auto yy : iter_all<neuron_on_block_type::y_type>()) {
		bitset_type current_row;
		auto it = m_chunks.lower_bound(
			std::make_pair(neuron_on_block_type(X(neuron_on_block_type::x_type::min), yy), 0u));
		auto end = m_chunks.upper_bound(
			std::make_pair(neuron_on_block_type(X(neuron_on_block_type::x_type::max), yy), 0u));

		for (; it != end; ++it) {
			neuron_on_block_type nrn;
			size_t size;
			std::tie(nrn, size) = *it;
			if (size == 0u) {
				throw std::invalid_argument(
					"neuron chunk has to be composed of at least one denmem");
			}
			if (nrn.x() + size - 1 > neuron_on_block_type::x_type::max) {
				throw std::invalid_argument("neuron dimensions exceed neuron block bounds");
			}
			m_size += size;

			bitset_type current_neuron;
			// [111111111]
			current_neuron.set();
			// Shift s.t. only `size` ones remain:
			// [111000000]
			current_neuron <<= (current_neuron.size() - size);
			// Shift to position `nrn.x()`:
			// [000011100]
			current_neuron >>= nrn.x();

			if ((current_row & current_neuron).any()) {
				throw std::invalid_argument("overlapping chunks in neuron");
			}

			current_row |= current_neuron;
		}

		// Check for connected components.
		// To ensure that there is a path from left to right that connects all chunks of
		// the current and last row, we start from the leftmost denmem and go along the
		// corresponding “active” row (either current or last) until the respective chunk
		// ends.  To further continue the path to the right it is necessary that the other
		// row now takes the role of “active” row.  This repeats until there are no
		// denmems further to the right.

		bitset_type const* active = nullptr;
		for (size_t xx = 0u; xx < neuron_on_block_type::x_type::end; ++xx) {
			// Find leftmost denmem and set active row.
			if (active == nullptr) {
				active = last_row[xx] ? &last_row : (current_row[xx] ? &current_row : nullptr);
				continue;
			}

			// Move until end of current chunk on active row.
			if ((*active)[xx]) {
				continue;
			}

			// As there are no more denmems connected to the chunk in the active row, the
			// other row has to be connected to the last denmem in this row and take over
			// the role of the active row.  Else there are no denmems allowed further to
			// the right.
			bitset_type const* other = (active == &current_row) ? &last_row : &current_row;
			if ((*other)[xx - 1] && (*other)[xx]) {
				active = other;
				continue;
			}

			// Check that there are no denmems further to the right.
			for (; xx < neuron_on_block_type::x_type::end; ++xx) {
				if (current_row[xx] || last_row[xx]) {
					throw std::invalid_argument("disconnected chunks in neuron");
				}
			}
			break;
		}

		last_row = current_row;
	}
}

bool LogicalNeuron::is_external() const
{
	return m_chunks.empty();
}

auto LogicalNeuron::external_identifier() const -> external_identifier_type
{
	if (!is_external()) {
		throw std::runtime_error("external identifier is only available for external neurons");
	}

	return m_external_identifier;
}

size_t LogicalNeuron::external_index() const
{
	if (!is_external()) {
		throw std::runtime_error("external index is only available for external neurons");
	}

	return m_external_index;
}

size_t LogicalNeuron::size() const
{
	if (is_external()) {
		throw std::runtime_error("size is not available for external neurons");
	}

	return m_size;
}

auto LogicalNeuron::denmem(size_t const index) const -> neuron_type
{
	if (is_external()) {
		throw std::runtime_error("external neuron does not have denmems");
	}

	size_t seen = 0u;
	for (auto const& chunk : m_chunks) {
		size_t const relative = index - seen;
		if (relative < chunk.second) {
			return neuron_on_block_type(X(chunk.first.x() + relative), Y(chunk.first.y()))
			    .toNeuronOnWafer(m_block);
		}
		seen += chunk.second;
	}

	throw std::out_of_range("denmem index out of bounds");
}

auto LogicalNeuron::begin() const -> iterator
{
	return {m_block, m_chunks.begin()};
}

auto LogicalNeuron::end() const -> iterator
{
	return {m_block, m_chunks.end()};
}

auto LogicalNeuron::front() const -> neuron_type
{
	if (is_external()) {
		throw std::runtime_error("external neuron does not have denmems");
	}

	return m_chunks.begin()->first.toNeuronOnWafer(m_block);
}

auto LogicalNeuron::back() const -> neuron_type
{
	if (is_external()) {
		throw std::runtime_error("external neuron does not have denmems");
	}

	auto const& chunk = *m_chunks.rbegin();
	return neuron_on_block_type(X(chunk.first.x() + chunk.second - 1), Y(chunk.first.y()))
		.toNeuronOnWafer(m_block);
}

bool LogicalNeuron::is_rectangular() const
{
	if (is_external()) {
		throw std::runtime_error("external neuron does not have denmems");
	}

	if (m_chunks.size() > neuron_on_block_type::y_type::size) {
		return false;
	}

	auto it = m_chunks.begin();
	auto next = std::next(it);

	for (auto end = m_chunks.end(); next != end; ++it, ++next) {
		if (next->first.x() != it->first.x() || next->first.y() <= it->first.y() ||
		    next->second != it->second) {
			return false;
		}
	}

	return true;
}

bool LogicalNeuron::shares_denmems_with(LogicalNeuron const& other) const
{
	if (is_external() || other.is_external()) {
		throw std::runtime_error("external neuron does not have denmems");
	}

	if (m_block != other.m_block) {
		return false;
	}

	auto a_it = m_chunks.begin();
	auto b_it = other.m_chunks.begin();
	auto const a_eit = m_chunks.end();
	auto const b_eit = other.m_chunks.end();

	while (a_it != a_eit && b_it != b_eit) {
		auto const& a_nrn = a_it->first;
		auto const& b_nrn = b_it->first;

		if (a_nrn.y() == b_nrn.y()) {
			auto* left = &a_it;
			auto const* right = &b_it;

			if (a_nrn.x() > b_nrn.x()) {
				left = &b_it;
				right = &a_it;
			}

			if (((*right)->first.x() - (*left)->first.x()) >= (*left)->second) {
				// right chunk has no overlap with left chunk.
				++(*left);
				continue;
			}

			return true;
		} else if (a_nrn.y() < b_nrn.y()) {
			++a_it;
		} else {
			++b_it;
		}
	}

	return false;
}

size_t LogicalNeuron::hash() const
{
	size_t hash = 0;
	boost::hash_combine(hash, m_external_identifier);
	boost::hash_combine(hash, m_size);
	boost::hash_combine(hash, m_block);
	boost::hash_combine(hash, m_chunks.size());
	// This is just a hash, so we don't bother looping over all chunks.
	if (!m_chunks.empty()) {
		chunk_type const& chunk = *(m_chunks.begin());
		boost::hash_combine(hash, chunk.first);
		boost::hash_combine(hash, chunk.second);
	}
	return hash;
}

std::ostream& operator<<(std::ostream& os, LogicalNeuron const& nrn)
{
	return os << pretty_printed(nrn);
}

bool operator==(LogicalNeuron const& lhs, LogicalNeuron const& rhs)
{
	return (
	    lhs.m_external_identifier == rhs.m_external_identifier &&
	    lhs.m_size == rhs.m_size && lhs.m_block == rhs.m_block &&
	    lhs.m_chunks.size() == rhs.m_chunks.size() &&
	    std::equal(lhs.m_chunks.begin(), lhs.m_chunks.end(), rhs.m_chunks.begin()));
}

size_t hash_value(LogicalNeuron const& nrn)
{
	return nrn.hash();
}

template <typename Archiver>
void LogicalNeuron::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("external_index", m_external_identifier)
	   & make_nvp("size_or_identifier", m_size)
	   & make_nvp("block", m_block)
	   & make_nvp("chunks", m_chunks);
	// clang-format on
}

} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::LogicalNeuron)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::LogicalNeuron)
