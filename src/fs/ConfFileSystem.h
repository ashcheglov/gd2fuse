#pragma once

#include <string>
#include "utils/decls.h"
#include "IFileSystem.h"
#include "control/IConfiguration.h"

IFileSystemPtr createConfigurationFS(const IConfigurationPtr &conf);
