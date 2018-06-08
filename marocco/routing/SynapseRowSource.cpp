#include "marocco/routing/SynapseRowSource.h"

namespace marocco {
namespace routing {

SynapseSource::SynapseSource (SynapseSource::projection_view_type const & view, size_t src_idx, size_t tgt_idx):
	mProjView(view), mSrc(src_idx), mTgt(tgt_idx) {}

SynapseSource::SynapseSource () {}

SynapseSource::projection_view_type const & SynapseSource::projection_view() const
{
	return mProjView;
}

size_t SynapseSource::src() const
{
	return mSrc;
}

size_t SynapseSource::tgt() const
{
	return mTgt;
}

bool SynapseSource::empty() const
{
	return !mProjView;
}
bool SynapseSource::operator== (SynapseSource const& rhs) const
{
	return mProjView == rhs.mProjView
		&& mSrc == rhs.mSrc
		&& mTgt == rhs.mTgt;
}

bool SynapseSource::operator!= (SynapseSource const& rhs) const
{
	return !(*this == rhs);
}


SynapseRowSource::Address const&
SynapseRowSource::prefix(size_t ii) const
{
	return mPrefix.at(ii);
}

SynapseRowSource::Address&
SynapseRowSource::prefix(size_t ii)
{
	return mPrefix.at(ii);
}

SynapseRowSource::SynapseMapping const& SynapseRowSource::synapses() const
{
	return mSynapses;
}

SynapseRowSource::SynapseMapping &      SynapseRowSource::synapses()
{
	return mSynapses;
}

SynapseRowSource::SynapseType const&
SynapseRowSource::type() const
{
	return mType;
}

bool SynapseRowSource::operator== (SynapseRowSource const& rhs) const
{
	return mPrefix == rhs.mPrefix
		&& mType == rhs.mType
		&& mSynapses == rhs.mSynapses;
}

bool SynapseRowSource::operator!= (SynapseRowSource const& rhs) const
{
	return !(*this == rhs);
}

SynapseRowSource::SynapseRowSource(SynapseType const& type) :
	mPrefix(),
	mType(type)
{}

SynapseRowSource::SynapseRowSource()
{}


DriverResult::DriverResult(VLineOnHICANN const& vline) :
	mLine(vline),
	mDrivers(),
	mRows()
{}

DriverResult::VLineOnHICANN const&
DriverResult::line() const
{
	return mLine;
}

bool DriverResult::from_adjacent() const
{
	auto it = mDrivers.begin();
	if (it==mDrivers.end()) {
		throw std::runtime_error("empty driver Result");
	}
	SynapseDriverOnHICANN const& primary = it->first;
	return mLine.toSideHorizontal() != primary.toSideHorizontal();
}

DriverResult::Rows&
DriverResult::rows()
{
	return mRows;
}

DriverResult::Rows const&
DriverResult::rows() const
{
	return mRows;
}

DriverResult::Drivers&
DriverResult::drivers()
{
	return mDrivers;
}

DriverResult::Drivers const&
DriverResult::drivers() const
{
	return mDrivers;
}

void DriverResult::check() const
{
	if (drivers().size() > 2) {
		throw std::runtime_error("too many primary drivers");
	}
}

} // namespace routing
} // namespace marocco
