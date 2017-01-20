#pragma once

#include <string>
#include "utils/decls.h"

class IOAuth2Process
{

public:

	virtual bool isFinished() =0;
	virtual std::string getError() =0;
	virtual bool startNativeApp() =0;
	virtual bool finishNativeApp(const std::string &authKey) =0;

	virtual ~IOAuth2Process() {}
};

G2F_DECLARE_PTR(IOAuth2Process);
