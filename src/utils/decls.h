#ifndef HELPER_H
#define HELPER_H

#include <memory>
#include <boost/filesystem.hpp>

#define G2F_DECLARE_PTR(T) typedef std::shared_ptr<T> T##Ptr; typedef std::unique_ptr<T> T##UPtr

template<typename T>
using uptr=std::unique_ptr<T>;

template<typename T>
using sptr=std::shared_ptr<T>;

#define G2F_GOOGLE_NS_SHORTHANDS namespace g_api=googleapis; \
	namespace g_cli=googleapis::client; \
	namespace g_utl=googleapis::util;

namespace fs=boost::filesystem;

#define G2F_APP_NAME	"gd2fuse"

// Stub for localization
#define G2FMESSAGE(message) message

#endif // HELPER_H
