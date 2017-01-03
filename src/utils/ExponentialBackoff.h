#pragma once

#include <random>

class ExponentialBackoff
{
public:
	ExponentialBackoff(size_t nTry, int seed=1);
	bool end();
	size_t nextTime();

private:
	std::minstd_rand _gen;
	std::uniform_int_distribution<> _dist;

	size_t _nTry;
	size_t _curr;
};

