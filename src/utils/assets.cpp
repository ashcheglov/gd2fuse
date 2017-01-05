#include "assets.h"
#include "error/G2FException.h"

MimePair splitMime(const std::string &fullMime)
{
	MimePair ret;

	std::string::size_type pos=fullMime.find('/');
	if(pos!=std::string::npos)
	{
		ret.first=fullMime.substr(0,pos);
		ret.second=fullMime.substr(pos+1);
	}
	else
		ret.first=fullMime;

	return ret;
}

fs::path fromPathIt(const fs::path::const_iterator &begin, const fs::path::const_iterator &end)
{
	fs::path::iterator it(begin);
	fs::path ret;
	for(;it!=end;++it)
		ret/=*it;
	return ret;
}
