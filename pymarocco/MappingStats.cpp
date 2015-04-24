#include "pymarocco/MappingStats.h"
#include "euter/projection.h"
#include <ostream>
#include "marocco/placement/LookupTable.h"

namespace pymarocco {

MappingStats::MappingStats() :
	timeSpentInParallelRegion(0),
	timeTotal(0),
	mSynapseLoss(0),
	mSynapseLossAfterWaferRouting(0),
	mSynapses(0),
	mNumPopulations(0),
	mNumProjections(0),
	mNumNeurons(0)
{}

void MappingStats::setSynapseLoss(size_t s)
{
	mSynapseLoss = s;
}

size_t MappingStats::getSynapseLoss() const
{
	return mSynapseLoss;
}

void MappingStats::setSynapseLossAfterWaferRouting(size_t s)
{
	mSynapseLossAfterWaferRouting = s;
}

size_t MappingStats::getSynapseLossAfterWaferRouting() const
{
	return mSynapseLossAfterWaferRouting;
}

void MappingStats::setSynapses(size_t s)
{
	mSynapses = s;
}

size_t MappingStats::getSynapses() const
{
	return mSynapses;
}

void MappingStats::setSynapsesSet(size_t s)
{
	mSynapsesSet = s;
}

size_t MappingStats::getSynapsesSet() const
{
	return mSynapsesSet;
}

void MappingStats::setNumPopulations(size_t s)
{
	mNumPopulations = s;
}

size_t MappingStats::getNumPopulations() const
{
	return mNumPopulations;
}

void MappingStats::setNumProjections(size_t s)
{
	mNumProjections = s;
}

size_t MappingStats::getNumProjections() const
{
	return mNumProjections;
}

void MappingStats::setNumNeurons(size_t s)
{
	mNumNeurons = s;
}

size_t MappingStats::getNumNeurons() const
{
	return mNumNeurons;
}

void MappingStats::setNeuronUsage(double v)
{
	mNeuronUsage = v;
}

double MappingStats::getNeuronUsage() const
{
	return mNeuronUsage;
}

void MappingStats::setSynapseUsage(double v)
{
	mSynapseUsage = v;
}

double MappingStats::getSynapseUsage() const
{
	return mSynapseUsage;
}

marocco::placement::bio_id
MappingStats::getBioId(marocco::placement::hw_id const id) {
	if (!mLookupTable)
		throw std::runtime_error("Reverse mapping/lookup table not available");
	return mLookupTable->at(id);
}

std::vector<marocco::placement::hw_id>
MappingStats::getHwId(marocco::placement::bio_id const id) {
	if (!mLookupTable)
		throw std::runtime_error("Reverse mapping/lookup table not available");
	return mLookupTable->at(id);
}

void MappingStats::setLookupTable(std::shared_ptr<class marocco::placement::LookupTable> lut)
{
	mLookupTable = lut;
}

std::ostream& MappingStats::operator<< (std::ostream& os) const
{
	os << "MappingStats {"
		<< "\n\tsynapse_loss: " << getSynapseLoss()
		<< " (" << ((getSynapseLoss() == 0)
			? 0
			: double(getSynapseLoss())/getSynapses()*100) << "%)"
		<< "\n\tsynapses: " << getSynapses()
		<< "\n\tsynapses set: " << getSynapsesSet()
		<< "\n\tsynapses lost: " << getSynapseLoss()
		<< "\n\tsynapses lost(l1): " << getSynapseLossAfterWaferRouting()
		<< "\n\tpopulations: " << getNumPopulations()
		<< "\n\tprojections: " << getNumProjections()
		<< "\n\tneurons: " << getNumNeurons()
		<< "}";
	return os;
}

std::ostream& operator<< (std::ostream& os, MappingStats const& ms)
{
	return ms.operator<< (os);
}

MappingStats::Matrix const&
MappingStats::getWeights(ProjectionId const proj) const
{
	return mWeights.at(proj);
}

MappingStats::Matrix&
MappingStats::getWeights(ProjectionId const proj)
{
	return mWeights[proj];
}

} // pymarocco
