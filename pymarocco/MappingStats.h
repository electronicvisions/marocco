#pragma once

#include <cstdlib>
#include <iosfwd>
#include <map>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/serialization/nvp.hpp>
#include "boost/serialization/ublas.hpp"

#include "marocco/placement/LookupTableData.h"

namespace ublas = boost::numeric::ublas;


#if !defined(PYPLUSPLUS)
#include <memory>
// fwd dcl 4tw
namespace marocco { namespace placement {
class LookupTable;
}}
#endif // !defined(PYPLUSPLUS)

namespace pymarocco {

class MappingStats
{
public:
	typedef float value_type;
	typedef ublas::matrix<value_type> Matrix;
	typedef size_t ProjectionId;
	typedef marocco::placement::bio_id bio_id;
	typedef marocco::placement::hw_id hw_id;

	MappingStats();

	size_t getSynapseLoss() const;
	size_t getSynapseLossAfterWaferRouting() const;
	size_t getSynapses() const;
	size_t getSynapsesSet() const;
	size_t getNumPopulations() const;
	size_t getNumProjections() const;
	size_t getNumNeurons() const;

	double getNeuronUsage() const;
	double getSynapseUsage() const;

	/// translate between hardware spike address (HICANNOnWafer +
	/// OutputBufferOnHICANN + / L1Address) and biological neuron id
	bio_id getBioId(hw_id const id);

	/// translate between NeuronOnWafer and biological neuron id
	bio_id getBioId(HMF::Coordinate::NeuronOnWafer const id);

	/// translate between biological and hardware spike addresses
	std::vector<hw_id> getHwId(bio_id const id);

	/// return a vector of coordinates of the denmems which comprises bio
	/// neuron 'id'
	std::vector<HMF::Coordinate::NeuronOnWafer> getDenmems(bio_id const id);

#if !defined(PYPLUSPLUS)
	void setSynapseLoss(size_t s);
	void setSynapseLossAfterWaferRouting(size_t s);
	void setSynapses(size_t s);
	void setSynapsesSet(size_t s);
	void setNumPopulations(size_t s);
	void setNumProjections(size_t s);
	void setNumNeurons(size_t s);

	void setNeuronUsage(double v);
	void setSynapseUsage(double v);

	/// provide reverse mapping data
	void setLookupTable(std::shared_ptr<marocco::placement::LookupTable>);
#endif

	size_t timeSpentInParallelRegion;
	size_t timeTotal;

	std::ostream& operator<< (std::ostream& os) const;

	friend std::ostream& operator<< (
		std::ostream& os, MappingStats const& ms);

	Matrix const& getWeights(ProjectionId proj) const;
#if !defined(PYPLUSPLUS)
	Matrix&       getWeights(ProjectionId proj);
#endif

private:
	size_t mSynapseLoss;
	size_t mSynapseLossAfterWaferRouting;
	size_t mSynapses;
	size_t mSynapsesSet;
	size_t mNumPopulations;
	size_t mNumProjections;
	size_t mNumNeurons;

	double mNeuronUsage;
	double mSynapseUsage;

	/// mapping of projection ids to weight matrices
	std::map<ProjectionId, Matrix> mWeights;

#if !defined(PYPLUSPLUS)
	/// reverse mapping data
	std::shared_ptr<marocco::placement::LookupTable> mLookupTable;
#endif

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("synapse_loss", mSynapseLoss)
		   & make_nvp("synapse_loss_after_wafer_routing", mSynapseLossAfterWaferRouting)
		   & make_nvp("synapses", mSynapses)
		   & make_nvp("populations", mNumPopulations)
		   & make_nvp("projections", mNumProjections)
		   & make_nvp("neurons", mNumNeurons)
		   & make_nvp("weights", mWeights)

		   & make_nvp("neuron_usage", mNeuronUsage)
		   & make_nvp("synapse_usage", mSynapseUsage);
	}
};

} // pymarocco
