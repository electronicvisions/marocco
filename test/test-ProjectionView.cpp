#include "test/common.h"
#include "euter/objectstore.h"
#include "euter/population_view.h"
#include "euter/projection_view.h"
#include "euter/fixedprobabilityconnector.h"
#include "euter/nativerandomgenerator.h"
#include <boost/make_shared.hpp>
#include <iostream>
#include <sstream>

template<typename T>
std::string printWeights(T const& t)
{
	std::ostringstream os;
	for (size_t ii=0; ii<t.size1(); ++ii) {
		for (size_t jj=0; jj<t.size2(); ++jj) {
			os << t(ii, jj) << " ";
		}
		os << "\n";
	}
	return os.str();
}

TEST(ProjectionView, WeightAccess)
{
	ObjectStore os;

	PopulationPtr pop0 = Population::create(os, 10+rand()%10, CellType::IF_cond_exp);
	PopulationPtr pop1 = Population::create(os, 10+rand()%10, CellType::IF_cond_exp);

	double weight = 1.0;
	auto con = boost::make_shared<FixedProbabilityConnector>(1, true, weight);
	auto rng = boost::make_shared<NativeRandomGenerator>();

	ProjectionPtr proj0 = Projection::create(os, pop0, pop1, con, rng);

	typedef ProjectionView::range_type Range;
	ProjectionView view(pop0, pop1, proj0, Range(0, pop0->size()), Range(0, pop1->size()));

	auto weights0 = view.getWeights();

	// std::cout << printWeights(weights0);

	// change some weights
	auto& weightsRef = proj0->getWeights().get();
	weightsRef(0, 0) = 42.;

	// make sure weights0 is just a view on the original weights
	ASSERT_FLOAT_EQ(42., weights0(0, 0)) << printWeights(weights0);


	// let's see how we can copy the weights
	Connector::matrix_type copy_weights = weights0;
	//ASSERT_EQ(weights0, copy_weights); // unfortunately no operator==

	ASSERT_EQ(weights0.size1(), copy_weights.size1());
	ASSERT_EQ(weights0.size2(), copy_weights.size2());

	// lets change some orignal weight and make sure, copied weights do not
	// change.
	weightsRef(0, 0) = 23.;
	ASSERT_FLOAT_EQ(42., copy_weights(0, 0)) << printWeights(weights0);
	ASSERT_FLOAT_EQ(23., weights0(0, 0)) << printWeights(weights0);
}

TEST(PopulationView, Dimensions)
{
	typedef boost::dynamic_bitset<> mask_type;

	ObjectStore os;

	size_t const N = 3;

	PopulationPtr pop0 = Population::create(os, N, CellType::IF_cond_exp);
	PopulationView view0(pop0, mask_type(N, 2));

	PopulationPtr pop1 = Population::create(os, N, CellType::IF_cond_exp);
	PopulationView view1(pop1, mask_type(N, 5));

	double weight = 1.0;
	auto con = boost::make_shared<FixedProbabilityConnector>(1, true, weight);
	auto rng = boost::make_shared<NativeRandomGenerator>();

	ProjectionPtr proj0 = Projection::create(os, view0, view1, con, rng);

	auto const& weights = proj0->getWeights().get();

	// this test shows two things. First, that only neurons not present in the
	// PopulationView have no representation as column or row in the weight
	// matrix. And Second, index1 counts over presynaptic neurons, while index2
	// counts over postsynaptic neurons.
	ASSERT_EQ(1u, weights.size1());
	ASSERT_EQ(2u, weights.size2());
}
