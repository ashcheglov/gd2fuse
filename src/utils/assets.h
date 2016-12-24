#ifndef ASSETS_H
#define ASSETS_H

#include <string>
#include <array>
#include <googleapis/client/util/date_time.h>
#include <googleapis/strings/stringpiece.h>
#include <boost/filesystem.hpp>

namespace g_api=googleapis;
namespace g_cli=googleapis::client;
namespace fs=boost::filesystem;

typedef std::pair<std::string,std::string> MimePair;

MimePair splitMime(const std::string& fullMime);

void gdt2timespec(const g_cli::DateTime &from,timespec &to);

typedef std::array<int8_t,16> MD5Signature;
void sp2md5sign(const g_api::StringPiece &str,MD5Signature &ret);


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

#endif // ASSETS_H
