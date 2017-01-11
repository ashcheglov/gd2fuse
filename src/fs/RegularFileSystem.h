#pragma once

#include <string>
#include "utils/decls.h"
#include "IFileSystem.h"
#include "providers/IDataProvider.h"
#include "control/IConfiguration.h"

IFileSystemPtr createRegularFS(const IDataProviderPtr &dp);
IFileSystemPtr createConfigurationFS(const IConfigurationPtr &conf);
