#pragma once

// ESTER MPI header
#include "mpi/config.h"

namespace marocco {

class BaseResult
{
public:
	typedef int rank_t;

	BaseResult(rank_t rank = MPI::COMM_WORLD.Get_rank()) :
		_rank(rank) {}
	virtual ~BaseResult () {}

	rank_t getRank() const { return _rank; }

private:
	rank_t _rank;
};

} // namespace marocco
