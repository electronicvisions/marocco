#pragma once

#include <random>

namespace marocco {

class RNG
{
public:
    typedef std::minstd_rand Engine;
    typedef Engine::result_type result_type;
    typedef int int_type;

    RNG(result_type seed = 0) :
        mGen(seed)
    {}

    result_type operator() ()
    {
        return mGen();
    }

    int_type operator() (int_type max, int_type min=0)
    {
        std::uniform_int_distribution<int_type> dist(min, max);
        return dist(mGen);
    }

    std::minstd_rand mGen;
};

} // namespace marocco
