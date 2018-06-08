#pragma once

#include <memory>
#include <random>
#include <type_traits>

#if !defined(NDEBUG)
#include <cstdio>
#define MONITOR_WORSE_ACCEPTANCE(E, Enext) \
	if (Enext>E) { \
		printf("accepted worse state, dE=%.2f\n", Enext-E); \
	}
#else
#define MONITOR_WORSE_ACCEPTANCE(E, Enext)
#endif

namespace {
struct True {};
struct False {};
}

template<typename T>
class has_energy
{
private:
	template<typename C>
	static True test(decltype(&C::energy));

	template<typename>
	static False test(...);

public:
	static const bool value =
		std::is_same<True, decltype(test<T>(nullptr))>::value;
};

template<typename T>
class has_next
{
private:
	template<typename C>
	static True test(decltype(&C::next));

	template<typename>
	static False test(...);

public:
	static const bool value =
		std::is_same<True, decltype(test<T>(nullptr))>::value;
};

template<typename State>
class Annealer;

template<typename Derived>
struct StateBase
{
	typedef double Float;
	//void next();
	//double energy() const;
	typedef Derived type;

	static Float P(Float const Eold, Float const Enew, size_t t) {
		return exp(-(Enew-Eold)/type::temperature(t));
	}

	static Float thresh() {
		return -1.;
	}

	static Float temperature(size_t /*unused*/) {
		return 1.;
	}
};

template<typename State>
class Annealer
{
public:
	static_assert(std::is_base_of<StateBase<State>, State>::value, "");
	static_assert(has_next<State>::value, "");
	static_assert(has_energy<State>::value, "");
	typedef typename State::Float Float;

	Annealer(State const& init) :
		mT(0),
		mCur(new State(init)),
		mCand(new State(init)),
		mBest(new State(init)),
		mBestE(mBest->energy())
	{}

	void run(size_t iter)
	{
		if (mBestE<=State::thresh())
			return;

		Float E = mCur->energy();
		while (iter)
		{
			// update state
			mCand->next();

			Float const Ec = mCand->energy();

			// if new state is best so far
			if (Ec < mBestE) {
				mBest = std::make_shared<State>(*mCand);
				mBestE = Ec;
				if (mBestE<=State::thresh())
					return;
			}

			// accept better states in any case and worse with some prob (0<=P<=1).
			if (Ec<E || State::P(E, Ec, mT)>random()) {
				MONITOR_WORSE_ACCEPTANCE(E, Ec);
				mCur.swap(mCand);
				E = Ec;
				mCand.reset(new State(*mCur));
			}

			mT++;
			iter--;
		}
	}

	size_t iterations() const {
		return mT;
	}

	std::shared_ptr<State const> get() const {
		return mBest;
	}

	Float energy() const {
		return mBest->energy();
	}

	Float current_energy() const {
		return mCur->energy();
	}

	static Float random() {
		static std::random_device rng;
		std::uniform_real_distribution<Float> dist(0., 1.);
		return dist(rng);
	}

private:
	size_t mT;

	std::unique_ptr<State> mCur;
	std::unique_ptr<State> mCand;
	std::shared_ptr<State const> mBest;

	double mBestE;
};
