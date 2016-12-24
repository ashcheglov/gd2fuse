cmake_minimum_required(VERSION 2.8.13)

project(gd2fuse)

option(gd2fuse_SHOW_TRACE "Show debugging messages" OFF)
if(gd2fuse_SHOW_TRACE)
  add_definitions(-DG2F_USE_LOG)
endif()

set(gd2fuse_GOOGLEAPIS_INSTALL_DIR /usr/local CACHE PATH "Google clientAPI installation folder")

add_subdirectory(src)
