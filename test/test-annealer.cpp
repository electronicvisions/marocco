#include "test/common.h"
#include "marocco/annealer.h"

struct IncompleteState :
	public StateBase<IncompleteState>
{};

struct MyState :
	public StateBase<MyState>
{
	MyState() :
		StateBase<MyState>(),
		state(1000)
	{}

	void next() {
		state = rand() % 500;
	}

	double energy() const {
		return state;
	}

	static double temperature(size_t) {
		return 1.;
	}

	size_t state;
};

TEST(Annealer, SFINAE)
{
	ASSERT_FALSE((has_energy<IncompleteState>::value));
	ASSERT_FALSE((has_next<IncompleteState>::value));
	ASSERT_TRUE((std::is_base_of<StateBase<IncompleteState>, IncompleteState>::value));

	ASSERT_TRUE((has_energy<MyState>::value));
	ASSERT_TRUE((has_next<MyState>::value));
}

TEST(Annealer, Random)
{
	typedef MyState State;
	State sp;
	Annealer<State> an(sp);

	for(size_t ii=0; ii<1000; ++ii) {
		ASSERT_LE(0, an.random());
		ASSERT_GE(1, an.random());
	}
}

TEST(Annealer, MyState)
{
	srand(time(0));
	typedef MyState State;
	State sp;
	Annealer<State> an(sp);
	ASSERT_NO_THROW(an.run(1000));

	// this might rarely fail
	EXPECT_GT(10u, an.get()->energy()) << an.get()->state;
}
