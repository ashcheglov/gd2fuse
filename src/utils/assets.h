#pragma once

#include <string>
#include <array>
#include <boost/filesystem.hpp>
#include <time.h>

namespace fs=boost::filesystem;

typedef std::pair<std::string,std::string> MimePair;

MimePair splitMime(const std::string& fullMime);

typedef std::array<int8_t,16> MD5Signature;

#define G2F_CLEAN_STAT(statbuf) memset(&statbuf,0,sizeof(struct stat))


/*
// TODO Patch to boost path (instead of fromPathIt)
// Answer to http://stackoverflow.com/questions/9418105/cannot-create-a-path-using-boostfilesystem
namespace boost { namespace filesystem { namespace path_traits {

	void convert(seq.c_str(), seq.c_str()+seq.size(), m_pathname)
	{
		@
	}
}}}
*/
fs::path fromPathIt(const fs::path::const_iterator &begin,const fs::path::const_iterator &end);

// timespec operate
#define CLEAR_TIMESPEC(ts)  do{ts.tv_sec=0; ts.tv_nsec=0;}while(0)
#define ISSET_TIMESPEC(ts)	(ts.tv_sec!=0)

bool operator==(const struct timespec &l,const  struct timespec &r);
bool operator!=(const struct timespec &l,const  struct timespec &r);
