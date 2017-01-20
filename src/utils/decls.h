#pragma once

#include <memory>
#include <boost/filesystem.hpp>

#define G2F_DECLARE_PTR(T) typedef std::shared_ptr<T> T##Ptr; typedef std::unique_ptr<T> T##UPtr

template<typename T>
using uptr=std::unique_ptr<T>;

template<typename T>
using sptr=std::shared_ptr<T>;

#define G2F_APP_NAME	"gd2fuse"

typedef int posix_error_code;

// Stub for localization
#define G2FMESSAGE(message) message

namespace fs=boost::filesystem;

enum TimeAttrib
{
	AccessTime,
	ModificationTime,
	ChangeTime
};

// timespec operate
#define CLEAR_TIMESPEC(ts)  do{ts.tv_sec=0; ts.tv_nsec=0;}while(0)
#define ISSET_TIMESPEC(ts)	(ts.tv_sec!=0)
