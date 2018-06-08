#include "test/common.h"
#include "marocco/routing/SynapseDriverSA.h"
#include "marocco/routing/SynapseRowIterator.h"
#include "marocco/Logger.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

namespace {
size_t init_drivers(DriverConfigurationState::IntervalList& list)
{
	size_t total = 0;
	for (size_t ii=0; ii<60; ++ii)  {
		//size_t const driver = 1+rand()%15;
		size_t const driver = 1+rand()%4;
		total += driver;
		list.push_back(DriverInterval(VLineOnHICANN(rand()%128), driver));
	}
	return total;
}
}

TEST(DriverSATest, Optimization)
{
	DriverConfigurationState::IntervalList list;
	size_t total = init_drivers(list);

	DriverConfigurationState state(list, left);

	Annealer<DriverConfigurationState> an(state);

	double const Ebefore = an.get()->energy();
	ASSERT_GE(total, an.get()->countDrivers()) << (*an.get());
	ASSERT_EQ(total, an.get()->countRequested());


	an.run(1000);
	double const Eafter = an.get()->energy();

	ASSERT_GE(total, an.get()->countDrivers());
	ASSERT_EQ(total, an.get()->countRequested());
	ASSERT_GT(Ebefore, Eafter) << "before: " << Ebefore << " after: " << Eafter
		<< "\n" << (*an.get());

	DriverConfigurationState result_state = *an.get();
	result_state.postProcess();
	//info(this) << "result_state: \n" << result_state;
	ASSERT_EQ(0, result_state.countOverlap()) << result_state;

	auto result = result_state.result(HICANNGlobal());
	ASSERT_FALSE(result.empty());
	for (auto const& entry : result)
	{
		for (auto const& assignment : entry.second)
		{
			auto const& drivers = assignment.drivers;
			auto const& primary = assignment.primary;
			ASSERT_FALSE(drivers.empty());
			auto it = drivers.find(primary);
			ASSERT_NE(drivers.end(), it);
		}
	}
}

TEST(DriverSATest, CountDrivers)
{
	static size_t const DEFAULT_MAX_CHAIN_LENGTH = 5;
	DriverConfigurationState::IntervalList list;
	size_t total = 0;
	for (size_t ii=0; ii<100; ++ii)  {
		size_t const driver = 1+rand()%(DEFAULT_MAX_CHAIN_LENGTH*2);
		total += driver;
		list.push_back(DriverInterval(VLineOnHICANN(rand()%128), driver));
	}

	DriverConfigurationState state(list, left);
	Annealer<DriverConfigurationState> an(state);
	ASSERT_EQ(total, an.get()->countDrivers()) << (*an.get());
	ASSERT_EQ(total, an.get()->countRequested());
	an.run(100);
	ASSERT_EQ(total, an.get()->countDrivers()) << (*an.get());
	ASSERT_EQ(total, an.get()->countRequested());
}

TEST(DriverSATest, Iterator0)
{
	std::vector<DriverAssignment> list;

	{
		DriverAssignment a;
		a.primary = SynapseDriverOnHICANN(Enum(1));
		for (size_t ii=0; ii<5; ++ii)
		{
			a.drivers.insert(SynapseDriverOnHICANN(Enum(1+2*ii)));
		}
		ASSERT_EQ(a.drivers.size(), 5);
		list.push_back(a);
	}

	{
		DriverAssignment a;
		a.primary = SynapseDriverOnHICANN(Enum(25));
		for (size_t ii=0; ii<5; ++ii)
		{
			a.drivers.insert(SynapseDriverOnHICANN(Enum(25+2*ii)));
		}
		ASSERT_EQ(a.drivers.size(), 5);
		list.push_back(a);
	}

	SynapseRowIterator first(list.begin(), list.end());
	SynapseRowIterator  last(  list.end(), list.end());

	size_t cnt=0;
	for (; first != last; ++first)
	{
		cnt++;
	}
	ASSERT_EQ(20, cnt);
}

TEST(DriverSATest, Iterator)
{
	size_t const outer = 1 + rand() % 15;
	size_t const inner = 1 + rand() %  5;

	std::vector<DriverAssignment> list;
	for (size_t ii=0; ii<outer; ++ii) {
		DriverAssignment a;
		SynapseDriverOnHICANN const templ{};
		for (size_t jj=0; jj<inner; ++jj) {
			a.drivers.insert(SynapseDriverOnHICANN(Y(templ.line()+2*jj), templ.toSideHorizontal()));
		}
		ASSERT_EQ(inner, a.drivers.size());
		list.push_back(a);
	}
	ASSERT_EQ(outer, list.size());


	SynapseRowIterator first(list.begin(), list.end());
	SynapseRowIterator  last(  list.end(), list.end());

	size_t cnt = 0;
	for (; first != last; ++first)
	{
		if (cnt%1000 == 999) {
			std::cout << "inifinite loop" << std::endl;
		}
		++cnt;
	}
	ASSERT_EQ(outer*inner*2, cnt);
}

TEST(DriverSATest, DISABLED_OptimizationPlot)
{
	using namespace boost::filesystem;

	// open up file
	path p = temp_directory_path() / unique_path("marocco_SA_%%%%");
	std::cout << "temporary file path for annealing curve: " << p << std::endl;
	std::ofstream file(p.string());
	file << "# iter energy\n";

	DriverConfigurationState::IntervalList list;
	init_drivers(list);

	DriverConfigurationState state(list, left);
	Annealer<DriverConfigurationState> an(state);

	size_t const N = 50000;
	double const E_start = an.get()->energy();
	for (size_t ii=0; ii<N+1; ++ii)
	{
		if (ii>0) {
			an.run(1);
		}
		double const E_best = an.energy();
		double const E_cur = an.current_energy();
		double const temp = state.temperature(ii);
		file << ii << " " << E_cur << " " << E_best << " " << temp << std::endl;
	}

	file.flush();
	file.close();
}

} // routing
} // marocco
