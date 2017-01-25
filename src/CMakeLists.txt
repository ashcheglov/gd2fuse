cmake_minimum_required(VERSION 2.8.11)

#set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -Weffc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-switch -Wno-unused-local-typedefs -Wno-unused-variable")
option(Boost_USE_STATIC_LIBS "Boost static libs" OFF)
option(Boost_USE_MULTITHREADED ON)
option(Boost_USE_STATIC_RUNTIME OFF)
option(Boost_NO_BOOST_CMAKE ON)
set(BOOST_ROOT /usr/local CACHE PATH "Boost installation dir")

find_package(PkgConfig)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

set(G2F_BOOST_COMPONENTS filesystem program_options)
if(gd2fuse_SHOW_TRACE)
  list(APPEND G2F_BOOST_COMPONENTS log)
endif()
find_package(Boost COMPONENTS ${G2F_BOOST_COMPONENTS})


set(SRC_LIST	main.cpp
                config.h

                cache/Cache.h
                cache/Cache.cpp

                control/Application.h
                control/Application.cpp
                control/FuseOpts.h
                control/ChainedCofiguration.cpp
                control/ChainedCofiguration.h
                control/Configuration.cpp
                control/Configuration.h
                control/IConfiguration.h

                control/paths/IPathManager.h
                control/paths/PathManager.h
                control/paths/PathManager.cpp
                control/paths/AbstractPathManager.h
                control/paths/AbstractPathManager.cpp

                control/props/AbstractFileStaticInitPropertiesList.cpp
                control/props/AbstractFileStaticInitPropertiesList.h
                control/props/AbstractMemoryStaticInitPropertiesList.cpp
                control/props/AbstractMemoryStaticInitPropertiesList.h
                control/props/AbstractStaticInitPropertiesList.cpp
                control/props/AbstractStaticInitPropertiesList.h
                control/props/IEventHandler.h
                control/props/IPropertiesList.h
                control/props/IPropertyDefinition.h
                control/props/IPropertyIterator.h

                control/types/IPropertyType.cpp
                control/types/IPropertyType.h
                control/types/IPropertyTypeEnumDef.h
                control/types/PropertyTypesRegistry.cpp
                control/types/PropertyTypesRegistry.h
                control/types/PropertyTypeBool.cpp
                control/types/PropertyTypeBool.h
                control/types/PropertyTypeEnum.cpp
                control/types/PropertyTypeEnum.h
                control/types/PropertyTypeOint.cpp
                control/types/PropertyTypeOint.h
                control/types/PropertyTypePath.cpp
                control/types/PropertyTypePath.h
                control/types/PropertyTypeUint.cpp
                control/types/PropertyTypeUint.h

                fs/ConfFileSystem.cpp
                fs/ContentManager.h
                fs/ContentManager.cpp
                fs/JoinedFileSystem.cpp
                fs/JoinedFileSystem.h
                fs/RegularFileSystem.h
                fs/RegularFileSystem.cpp
                fs/IContentHandle.cpp
                fs/IContentHandle.h
                fs/IFileSystem.h
                fs/IDirectoryIterator.h
                fs/INode.h

                presentation/FuseGate.h
                presentation/FuseGate.cpp
                presentation/handler.cpp
                presentation/handler.h

                providers/IConversionDescription.h
                providers/IConversionIterator.h
                providers/IConvertFormat.h
                providers/IDataProvider.h
                providers/IDataReader.h
                providers/IIdList.h
                providers/IMetaWrapper.h
                providers/IOAuth2Process.h
                providers/IProvider.h
                providers/IProviderSession.h
                providers/ISupportedConversion.h
                providers/ProvidersRegistry.cpp
                providers/ProvidersRegistry.h

                providers/google/Auth.h
                providers/google/Auth.cpp
                providers/google/GoogleProvider.cpp
                providers/google/GoogleProvider.h

                error/appError.h
                error/appError.cpp
                error/G2FException.h
                error/G2FException.cpp

                utils/assets.h
                utils/assets.cpp
                utils/decls.h
                utils/Enum.h
                utils/ExponentialBackoff.h
                utils/ExponentialBackoff.cpp
                utils/log.h
                utils/log.cpp

                )

#set_property(SOURCE error/G2FException.cpp PROPERTY COMPILE_FLAGS -fdump-tree-original)

##aux_source_directory(. SRC_LIST)
#aux_source_directory(fuse SRC_LIST)

#find_package(Qt5Core)
#find_package(Qt5Network)

set(appName ${PROJECT_NAME})


pkg_search_module(FUSE REQUIRED fuse)

set(GAPI_INCLUDE_DIR ${gd2fuse_GOOGLEAPIS_INSTALL_DIR}/include)
set(GAPI_LIBRARY_DIR ${gd2fuse_GOOGLEAPIS_INSTALL_DIR}/lib)
set(GAPI_LIBRARY_LIST googleapis_curl_http
    # order is important
    google_drive_api
    googleapis_openssl_codec
    googleapis_scribes
    glog
    googleapis_oauth2
    googleapis_utils
    googleapis_jsoncpp
    googleapis_jsonplayback
    googleapis_internal
    jsoncpp
    googleapis_http
    googleapis_json
    googleapis_utils


    )
link_directories(${GAPI_LIBRARY_DIR})

add_executable(${appName} ${SRC_LIST})

set_property(TARGET ${appName} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${appName} PROPERTY CXX_STANDARD_REQUIRED ON)

#target_link_libraries(gd2fuse Qt5::Core Qt5::Network)
target_link_libraries(${appName} ${FUSE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${GAPI_LIBRARY_LIST})
target_include_directories(${appName} PUBLIC ${FUSE_INCLUDE_DIRS} ${GAPI_INCLUDE_DIR})
target_compile_options(${appName} PUBLIC ${FUSE_CFLAGS_OTHER})

if(CURL_FOUND)
    target_include_directories(${appName} PUBLIC ${CURL_INCLUDE_DIRS})
    target_link_libraries(${appName} ${CURL_LIBRARIES})
endif()

if(Boost_FOUND)
    target_include_directories(${appName} PUBLIC ${Boost_INCLUDE_DIR})
    target_link_libraries(${appName} ${Boost_LIBRARIES})
endif()

# The most simple way for a CMake user to tell cmake(1) to search in a non-standard prefix
# for a package is to set the CMAKE_PREFIX_PATH cache variable.
