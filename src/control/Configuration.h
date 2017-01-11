#pragma once

#include "IConfiguration.h"

IConfigurationPtr createGlobalConfiguration(const fs::path &manualConf);

