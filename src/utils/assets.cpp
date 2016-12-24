#include "assets.h"
#include "error/G2FException.h"
#include <boost/algorithm/hex.hpp>

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

void gdt2timespec(const g_cli::DateTime &from, timespec &to)
{
	struct timeval tv;
	from.GetTimeval(&tv);
	to.tv_sec=tv.tv_sec;
	to.tv_nsec=tv.tv_usec*1000;
}

void sp2md5sign(const g_api::StringPiece &str, MD5Signature &ret)
{
	if(str.size()!=32)
		throw G2F_EXCEPTION("Can't recognize MD5 hash. Wrong length =%1, must be %2").arg(str.size()).arg(32);
	boost::algorithm::unhex(str.begin(),str.end(),ret.begin());
}

fs::path fromPathIt(const fs::path::const_iterator &begin, const fs::path::const_iterator &end)
{
	fs::path::iterator it(begin);
	fs::path ret;
	for(;it!=end;++it)
		ret/=*it;
	return ret;
}
