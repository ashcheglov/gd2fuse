#pragma once

#include <string>
#include <array>
#include <boost/filesystem.hpp>

namespace fs=boost::filesystem;

typedef std::pair<std::string,std::string> MimePair;

MimePair splitMime(const std::string& fullMime);

typedef std::array<int8_t,16> MD5Signature;


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

