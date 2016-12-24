#include "ExponentialBackoff.h"

ExponentialBackoff::ExponentialBackoff(size_t nTry,int seed)
	:_nTry(nTry),
	  _curr(0),
	  _gen(seed),
	  _dist(1,1000)
{}

bool ExponentialBackoff::end()
{
	return _curr>=_nTry;
}

size_t ExponentialBackoff::nextTime()
{
	size_t ret=(1 << _curr++)*1000 + _dist(_gen);
}
