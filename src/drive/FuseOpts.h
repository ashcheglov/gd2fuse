#ifndef FUSEOPTS_H
#define FUSEOPTS_H

#include "utils/decls.h"
#include <string>
#include <vector>

struct FUSEOpts
{
	FUSEOpts()
		: debug(false),
		  foreground(false),
		  disableMThread(false)
	{}

	typedef std::vector<std::string> O0oList;
	bool debug;
	bool foreground;
	bool disableMThread;
	fs::path mountPoint;
	O0oList ooo;
};

#endif // FUSEOPTS_H
