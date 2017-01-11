#pragma once

#include "IPathManager.h"


// Implements XDG Base Directory Specification (see https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html)
IPathManagerPtr createXDGPathManager();

// Implements directory structure under single directory by user query
IPathManagerPtr createSinglePointPathManager(const fs::path &root);

// Returns wrapper around parent that adds nextLevel after each returned value
IPathManagerPtr createNextLevelWrapperPathManager(const IPathManagerPtr &parent, const fs::path &nextLevel);
