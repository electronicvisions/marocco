#include <random>
#include <set>
#include <unordered_set>

#include <gtest/gtest.h>

#include "hal/Coordinate/HMFGeometry.h"
#include "redman/backend/MockBackend.h"

#include "marocco/resource/Manager.h"

template <typename... T>
void unused(T&&...) {}

namespace marocco {
namespace resource {

namespace Co = HMF::Coordinate;

void disable_hicann(boost::shared_ptr<redman::backend::Backend> backend,
                    Co::HICANNGlobal r) {
	Co::Wafer wafer;
	Co::HICANNOnWafer hicann;
	std::tie(wafer, hicann) = r.split();

	redman::resources::WaferWithBackend wman(backend, wafer);
	wman.hicanns()->disable(hicann);
	wman.save();
}

class TestWithMockBackend : public ::testing::Test {
public:
	TestWithMockBackend()
		: backend(boost::make_shared<redman::backend::MockBackend>()) {}
	boost::shared_ptr<redman::backend::Backend> backend;
};

class AHICANNManager : public TestWithMockBackend {
public:
	AHICANNManager()
		: wafer(), manager(backend, std::set<Co::Wafer>{wafer}) {}

	void reload() {
		manager.reload();
	}

	Co::Wafer wafer;
	HICANNManager manager;
};

class AHICANNManagerWithHICANN : public AHICANNManager,
                                 public ::testing::WithParamInterface<size_t> {
public:
	AHICANNManagerWithHICANN()
		: hicann(Co::HICANNOnWafer(Co::Enum(GetParam())), wafer) {};

	Co::HICANNGlobal hicann;
};

enum class OmittingMethod {
	RESOURCE_MANAGER,
	MASK
};

INSTANTIATE_TEST_CASE_P(
	AllPossibleHicanns,
	AHICANNManagerWithHICANN,
	::testing::Range(
		Co::HICANNOnWafer::enum_type::begin,
		Co::HICANNOnWafer::enum_type::end),);

class AHICANNManagerWithOmittedHICANN
    : public AHICANNManager,
      public ::testing::WithParamInterface< ::testing::tuple<OmittingMethod, size_t>> {
public:
	AHICANNManagerWithOmittedHICANN()
		: hicann(Co::HICANNOnWafer(Co::Enum(::testing::get<1>(GetParam()))), wafer)
	{
		switch (::testing::get<0>(GetParam())) {
			case OmittingMethod::RESOURCE_MANAGER:
				disable_hicann(backend, hicann);
				reload();
				break;
			case OmittingMethod::MASK:
				manager.mask(hicann);
				break;
		}
	}

	Co::HICANNGlobal hicann;
};

INSTANTIATE_TEST_CASE_P(
	AllPossibleHicanns,
	AHICANNManagerWithOmittedHICANN,
	::testing::Combine(
		::testing::Values(
			OmittingMethod::RESOURCE_MANAGER,
			OmittingMethod::MASK),
		::testing::Range(
			Co::HICANNOnWafer::enum_type::begin,
			Co::HICANNOnWafer::enum_type::end)),);

class AHICANNManagerWithRandomlyAllocatedHICANNS
	: public AHICANNManager,
	  public ::testing::WithParamInterface<size_t> {
public:
	AHICANNManagerWithRandomlyAllocatedHICANNS() {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<size_t> dist{
			Co::HICANNOnWafer::enum_type::min,
			Co::HICANNOnWafer::enum_type::max};

		for (size_t ii = 0; ii < GetParam(); ++ii) {
			while (true) {
				size_t id = dist(gen);
				auto hicann = Co::HICANNGlobal{Co::HICANNOnWafer(Co::Enum(id)), wafer};
				if (manager.available(hicann)) {
					allocated.insert(hicann);
					manager.allocate(hicann);
					break;
				}
			}
		}
	}

	std::unordered_set<Co::HICANNGlobal> allocated;
};

INSTANTIATE_TEST_CASE_P(
	ManyDiceRolls,
	AHICANNManagerWithRandomlyAllocatedHICANNS,
	::testing::Range(
		Co::HICANNOnWafer::enum_type::min,
		Co::HICANNOnWafer::enum_type::size),);

TEST_F(AHICANNManager, AcceptsASetOfWafersOnConstruction) {
	std::set<Co::Wafer> set;
	HICANNManager manager{backend, set};
}

TEST_F(AHICANNManager, AllowsToLoadWaferDataLater) {
	manager.load(Co::Wafer(42));
}

TEST_F(AHICANNManager, AllowsToLoadWaferDataOnlyOnce) {
	ASSERT_ANY_THROW(manager.load(Co::Wafer()));
}

TEST_F(AHICANNManager, ReturnsLoadedWafers) {
	ASSERT_EQ(1, manager.wafers().size());
	ASSERT_EQ(Co::Wafer{}, manager.wafers().front());

	manager.load(Co::Wafer{42});
	ASSERT_EQ(2, manager.wafers().size());
}

TEST_P(AHICANNManagerWithHICANN, CanCheckIfHicannIsPresent) {
	Co::HICANNGlobal different_wafer{Co::HICANNOnWafer{Co::Enum{2}}, Co::Wafer{42}};

	ASSERT_TRUE(manager.has(hicann));
	ASSERT_FALSE(manager.has(different_wafer));
}

TEST_F(AHICANNManager, AllowsToInjectWaferResources) {
	redman::resources::WaferWithBackend res{backend, Co::Wafer{42}};
	Co::HICANNGlobal absent{Co::HICANNOnWafer{Co::Enum{2}}, Co::Wafer{42}};
	res.hicanns()->disable(absent.toHICANNOnWafer());
	ASSERT_NO_THROW(manager.inject(res));
	ASSERT_FALSE(manager.has(absent));
}

TEST_P(AHICANNManagerWithHICANN, CanMaskHicanns) {
	ASSERT_TRUE(manager.has(hicann));
	ASSERT_FALSE(manager.masked(hicann));
	ASSERT_NO_THROW(manager.mask(hicann));
	ASSERT_TRUE(manager.masked(hicann));
	ASSERT_FALSE(manager.has(hicann));
	ASSERT_FALSE(manager.available(hicann));
	ASSERT_ANY_THROW(manager.mask(hicann));
}

TEST_P(AHICANNManagerWithHICANN, CanUnmaskHicanns) {
	manager.mask(hicann);
	ASSERT_TRUE(manager.masked(hicann));
	ASSERT_NO_THROW(manager.unmask(hicann));
	ASSERT_FALSE(manager.masked(hicann));
	ASSERT_TRUE(manager.has(hicann));
}

TEST_P(AHICANNManagerWithHICANN, CanAllocateHicanns) {
	ASSERT_NO_THROW(manager.allocate(hicann));
}

TEST_P(AHICANNManagerWithHICANN, ThrowsOnMultipleAllocations) {
	ASSERT_NO_THROW(manager.allocate(hicann));
	ASSERT_ANY_THROW(manager.allocate(hicann));
}

TEST_F(AHICANNManager, CanAllocateOnlyHicannsOnExistingWafers) {
	Co::HICANNGlobal hicann{Co::HICANNOnWafer{Co::Enum{42}}, Co::Wafer{42}};
	ASSERT_ANY_THROW(manager.allocate(hicann));
}

TEST_P(AHICANNManagerWithHICANN, CanCheckAvailabilityOfHicann) {
	ASSERT_TRUE(manager.available(hicann));
	manager.allocate(hicann);
	ASSERT_FALSE(manager.available(hicann));
}

TEST_F(AHICANNManager, CanReloadWithoutForgettingWafers) {
	Co::HICANNGlobal hicann{Co::HICANNOnWafer{Co::Enum{42}}, Co::Wafer{42}};
	manager.load(hicann.toWafer());
	ASSERT_TRUE(manager.has(hicann));
	reload();
	ASSERT_TRUE(manager.has(hicann));
}

TEST_P(AHICANNManagerWithHICANN, CanReloadWithoutMessingUpAllocation) {
	manager.allocate(hicann);
	ASSERT_FALSE(manager.available(hicann));
	reload();
	ASSERT_FALSE(manager.available(hicann));
}

TEST_P(AHICANNManagerWithHICANN, CanReleaseAllocatedHicanns) {
	ASSERT_TRUE(manager.available(hicann));
	manager.allocate(hicann);
	ASSERT_FALSE(manager.available(hicann));
	manager.release(hicann);
	ASSERT_TRUE(manager.available(hicann));
}

TEST_P(AHICANNManagerWithHICANN, ThrowsOnReleaseWithoutAllocation) {
	ASSERT_ANY_THROW(manager.release(hicann));
}

TEST_P(AHICANNManagerWithHICANN, KnowsTheNumberOfHicanns) {
	auto const size = Co::HICANNOnWafer::enum_type::size;
	ASSERT_EQ(size, manager.count_present());
	ASSERT_EQ(size, manager.count_available());
	manager.allocate(hicann);
	ASSERT_EQ(size, manager.count_present());
	ASSERT_EQ(size - 1, manager.count_available());
}

TEST_P(AHICANNManagerWithHICANN, KnowsTheNumberOfAllocatedHicanns) {
	ASSERT_EQ(0, manager.count_allocated());
	manager.allocate(hicann);
	ASSERT_EQ(1, manager.count_allocated());
	manager.release(hicann);
	ASSERT_EQ(0, manager.count_allocated());
}

TEST_P(AHICANNManagerWithHICANN, CanLoadTheResourceManagerOfHicann) {
	ASSERT_TRUE(static_cast<bool>(manager.get(hicann)));
}

TEST_P(AHICANNManagerWithOmittedHICANN, ThrowsWhenAllocating) {
	ASSERT_ANY_THROW(manager.allocate(hicann));
}

TEST_P(AHICANNManagerWithOmittedHICANN, ThrowsWhenTryingToMask) {
	ASSERT_ANY_THROW(manager.mask(hicann));
}

TEST_P(AHICANNManagerWithOmittedHICANN, DoesNotShowHicannAsAvailable) {
	ASSERT_FALSE(manager.available(hicann));
}

TEST_P(AHICANNManagerWithOmittedHICANN, DoesNotShowHicannAsPresent) {
	ASSERT_FALSE(manager.has(hicann));
}

TEST_P(AHICANNManagerWithOmittedHICANN, ThrowsWhenLoadingResourceManagerOfHicann) {
	ASSERT_ANY_THROW(manager.get(hicann));
}

TEST_P(AHICANNManagerWithOmittedHICANN, HasFewerHicanns) {
	auto const size = Co::HICANNOnWafer::enum_type::size;
	ASSERT_EQ(size - 1, manager.count_present());
	ASSERT_EQ(size - 1, manager.count_available());
}

TEST_F(AHICANNManager, CanBeIteratedOver) {
	size_t count = 0;

	for (auto hicann : manager.available()) {
		ASSERT_EQ(wafer, hicann.toWafer());
		++count;
	}

	for (auto hicann : manager.allocated()) {
		unused(hicann);
		FAIL() << "No HICANNs allocated yet.";
	}

	ASSERT_EQ(Co::HICANNOnWafer::enum_type::size, count);

	for (auto hicann : manager.present()) {
		ASSERT_EQ(wafer, hicann.toWafer());
		--count;
	}

	ASSERT_EQ(0, count);
}

TEST_F(AHICANNManager, CanBeIteratedOverMultipleTimes) {
	size_t count = 0;

	for (auto hicann : manager.available()) {
		unused(hicann);
		++count;
	}

	for (auto hicann : manager.available()) {
		unused(hicann);
		++count;
	}

	ASSERT_EQ(Co::HICANNOnWafer::enum_type::size * 2, count);

	for (auto hicann : manager.present()) {
		ASSERT_EQ(wafer, hicann.toWafer());
		--count;
	}

	for (auto hicann : manager.present()) {
		ASSERT_EQ(wafer, hicann.toWafer());
		--count;
	}

	ASSERT_EQ(0, count);
}

TEST_F(AHICANNManager, AllowsAllocationDuringIteration) {
	size_t count = 0;

	for (auto hicann : manager.available()) {
		manager.allocate(hicann);
		auto id = hicann.toHICANNOnWafer().toEnum();
		if (id > 0) {
			ASSERT_FALSE(manager.available(Co::HICANNGlobal{Co::HICANNOnWafer{Co::Enum{id - 1}}, wafer}));
		}
		++count;
	}

	auto const size = Co::HICANNOnWafer::enum_type::size;
	ASSERT_EQ(size, count);
	ASSERT_EQ(size, manager.count_allocated());
}

TEST_F(AHICANNManager, AllowsAllocationOfYetToComeHicannsDuringIteration) {
	size_t count = 0;
	auto const size = Co::HICANNOnWafer::enum_type::size;

	for (auto hicann : manager.available()) {
		auto id = hicann.toHICANNOnWafer().toEnum();
		if (id < size - 1) {
			manager.allocate(Co::HICANNGlobal{Co::HICANNOnWafer{Co::Enum{id + 1}}, wafer});
		}
		++count;
	}

	ASSERT_EQ(size/2, count);
	ASSERT_EQ(size/2, manager.count_allocated());
}

TEST_F(AHICANNManager, AllowsMaskingDuringIteration) {
	size_t count = 0;

	for (auto hicann : manager.present()) {
		manager.mask(hicann);
		auto id = hicann.toHICANNOnWafer().toEnum();
		if (id > 0) {
			ASSERT_FALSE(manager.has(Co::HICANNGlobal{Co::HICANNOnWafer{Co::Enum{id - 1}}, wafer}));
		}
		++count;
	}

	auto const size = Co::HICANNOnWafer::enum_type::size;
	ASSERT_EQ(size, count);
	ASSERT_EQ(0, manager.count_present());
}

TEST_F(AHICANNManager, AllowsMaskingOfYetToComeHicannsDuringIteration) {
	size_t count = 0;
	auto const size = Co::HICANNOnWafer::enum_type::size;

	for (auto hicann : manager.present()) {
		auto id = hicann.toHICANNOnWafer().toEnum();
		if (id < size - 1) {
			manager.mask(Co::HICANNGlobal{Co::HICANNOnWafer{Co::Enum{id + 1}}, wafer});
		}
		++count;
	}

	ASSERT_EQ(size/2, count);
	ASSERT_EQ(size/2, manager.count_present());
}

TEST_F(AHICANNManager, CanBeIteratedOverMultipleWafersWooooaaaah) {
	Co::Wafer other{42};
	manager.load(other);

	size_t count = 0;

	for (auto hicann : manager.available()) {
		ASSERT_TRUE(hicann.toWafer() == wafer || hicann.toWafer() == other);
		++count;
	}

	ASSERT_EQ(Co::HICANNOnWafer::enum_type::size * 2, count);
}

TEST_P(AHICANNManagerWithOmittedHICANN, OmitsHicannWhenIterating) {
	size_t count = 0;

	for (auto other : manager.available()) {
		ASSERT_NE(hicann, other);
		++count;
	}

	ASSERT_EQ(Co::HICANNOnWafer::enum_type::size - 1, count);
}

TEST_P(AHICANNManagerWithOmittedHICANN, OmitsHicannWhenIteratingPresentHicanns) {
	size_t count = 0;

	for (auto other : manager.present()) {
		ASSERT_NE(hicann, other);
		++count;
	}

	ASSERT_EQ(Co::HICANNOnWafer::enum_type::size - 1, count);
}

TEST_P(AHICANNManagerWithHICANN, OmitsAllocatedHicannWhenIterating) {
	size_t count = 0;

	manager.allocate(hicann);
	for (auto other : manager.available()) {
		ASSERT_NE(hicann, other);
		++count;
	}

	ASSERT_EQ(Co::HICANNOnWafer::enum_type::size - 1, count);
}

TEST_P(AHICANNManagerWithRandomlyAllocatedHICANNS, HasFewerHicannsAvailable) {
	ASSERT_EQ(manager.count_present() - GetParam(), manager.count_available());

	size_t count = 0;

	for (auto hicann : manager.available()) {
		unused(hicann);
		++count;
	}

	ASSERT_EQ(manager.count_available(), count);
}

TEST_P(AHICANNManagerWithRandomlyAllocatedHICANNS, OmitsAllocatedHicannWhenIterating) {
	for (auto hicann : manager.available()) {
		ASSERT_TRUE(allocated.find(hicann) == allocated.end());
	}
}

TEST_P(AHICANNManagerWithRandomlyAllocatedHICANNS, CanIterateOverAllocatedHicanns) {
	size_t count = 0;

	for (auto hicann : manager.allocated()) {
		ASSERT_TRUE(allocated.find(hicann) != allocated.end());
		++count;
	}

	ASSERT_EQ(allocated.size(), count);
}

} // resource
} // marocco
