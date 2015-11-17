#include <array>
#include <vector>

#include "marocco/util/chunked.h"
#include "test/common.h"

namespace marocco {

static_assert(std::is_same<detail::iterator_of<std::vector<size_t>>::type,
                           std::vector<size_t>::iterator>::value,
              "detail::iterator_of has wrong type for vector of size_t.");

TEST(ChunkedIterator, AcceptsIterators)
{
	std::array<size_t, 12> values{{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4}};
	for (auto& chunk : chunked(values.begin(), values.end(), 3)) {
		EXPECT_EQ(3, chunk.size());
	}
}

TEST(ChunkedIterator, AcceptsContainers)
{
	std::array<size_t, 12> values{{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4}};
	for (auto& chunk : chunked(values, 3)) {
		EXPECT_EQ(3, chunk.size());
	}
}

TEST(ChunkedIterator, OfSizeZeroDoesNothing)
{
	std::array<size_t, 12> values{{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4}};
	for (auto& chunk : chunked(values, 0)) {
		FAIL() << chunk.size();
	}
}

TEST(ChunkedIterator, WorksOnArray)
{
	std::array<size_t, 12> values{{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4}};
	size_t index = 0;
	for (auto& chunk : chunked(values, 3)) {
		EXPECT_EQ(3, chunk.size());
		EXPECT_EQ(index, chunk.index());

		++index;

		{
			size_t sum = 0;
			for (auto num : chunk) {
				EXPECT_EQ(index, num);
				sum += num;
			}
			EXPECT_EQ(3 * index, sum);
		}

		// we should be able to iterate several times
		{
			size_t sum = 0;
			for (auto num : chunk) {
				EXPECT_EQ(index, num);
				sum += num;
			}
			EXPECT_EQ(3 * index, sum);
		}
	}
	EXPECT_EQ(4, index);
}

TEST(ChunkedIterator, CanHaveShortTrailingChunk)
{
	std::array<size_t, 12> values{{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4}};
	for (auto& chunk : chunked(values, 5)) {
		EXPECT_EQ((chunk.index() < 2) ? 5 : 2, chunk.size());
	}
}

TEST(ChunkedIterator, WorksForHandUnrolledCode)
{
	std::vector<size_t> target{1, 2, 3, 4, 5, 6, 7};
	detail::chunked<typename std::vector<size_t>::iterator> wrap(target.begin(), target.end(), 3);
	auto bit = wrap.begin();
	auto& chunk = *bit;
	EXPECT_FALSE(chunk.empty());
	EXPECT_EQ(0, chunk.index());
	EXPECT_EQ(3, chunk.size());

	auto it = chunk.begin();
	EXPECT_EQ(1, *it++);
	EXPECT_EQ(2, *it++);
	EXPECT_EQ(3, *it++);

	++bit;
	chunk = *bit;
	EXPECT_FALSE(chunk.empty());
	EXPECT_EQ(1, chunk.index());
	EXPECT_EQ(3, chunk.size());

	it = chunk.begin();
	EXPECT_EQ(4, *it++);
	EXPECT_EQ(5, *it++);
	EXPECT_EQ(6, *it++);

	++bit;
	chunk = *bit;
	EXPECT_FALSE(chunk.empty());
	EXPECT_EQ(2, chunk.index());
	EXPECT_EQ(1, chunk.size());

	it = chunk.begin();
	EXPECT_EQ(7, *it++);

	++bit;
	EXPECT_EQ(wrap.end(), bit);
}

TEST(ChunkedIterator, IteratesAllValuesInOrderOldStyleLoop)
{
	std::vector<size_t> values;
	std::vector<size_t> target{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

	auto xit = target.cbegin();
	auto wrap = chunked(target.begin(), target.end(), 3);
	for (auto it = wrap.begin(), eit = wrap.end(); it != eit; ++it) {
		auto& chunk = *it;
		for (auto nit = chunk.begin(), neit = chunk.end(); nit != neit; ++nit) {
			auto num = *nit;
			EXPECT_EQ(*xit++, num);
			values.push_back(num);
		}
	}

	EXPECT_EQ(target, values);
}

TEST(ChunkedIterator, IteratesAllValuesInOrder)
{
	std::vector<size_t> values;
	std::vector<size_t> target{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

	auto it = target.cbegin();
	for (auto& chunk : chunked(target, 3)) {
		for (auto num : chunk) {
			EXPECT_EQ(*it++, num);
			values.push_back(num);
		}
	}

	EXPECT_EQ(target, values);
}

TEST(ChunkedIterator, CanModifyThroughIterator)
{
	std::array<size_t, 12> values;
	std::array<size_t, 12> target{{0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3}};
	for (auto& chunk : chunked(values, 3)) {
		for (size_t& num : chunk) {
			num = chunk.index();
			EXPECT_EQ(chunk.index(), num);
		}
	}

	EXPECT_EQ(target, values);
}

} // namespace marocco
