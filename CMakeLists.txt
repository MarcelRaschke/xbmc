cmake_minimum_required(VERSION 3.18)
if(WIN32)
  # Version 3.20 is required for multi-config generator expressions to work
  cmake_minimum_required(VERSION 3.20)
endif()
project(kodi LANGUAGES CXX C ASM)

if(POLICY CMP0069)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
  cmake_policy(SET CMP0069 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0074.html
# find_package() uses <PackageName>_ROOT variables
if(POLICY CMP0074)
  set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
  cmake_policy(SET CMP0074 NEW)
endif()

if(POLICY CMP0079)
  set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)
  cmake_policy(SET CMP0079 NEW)
endif()

if(POLICY CMP0135)
  set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
  cmake_policy(SET CMP0135 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0144.html
# find_package() uses upper-case <PACKAGENAME>_ROOT variables
if(POLICY CMP0144)
  set(CMAKE_POLICY_DEFAULT_CMP0144 NEW)
  cmake_policy(SET CMP0144 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules/buildtools)
if(DEPENDS_DIR)
  list(APPEND CMAKE_PREFIX_PATH ${DEPENDS_DIR})
endif()

# Variable to indicate if the project is targeting a Multi Config Generator (VS/Xcode primarily)
get_property(_multiconfig_generator GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_multiconfig_generator)
  # Target to encompass an optional way to build all internal dependencies
  # Must set EXCLUDE_FROM_ALL property so the ALL target doesnt include this, and therefore
  # build all internal dependencies
  add_custom_target(build_internal_depends)
  set_target_properties(build_internal_depends PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Set CORE_BUILD_DIR
set(CORE_BUILD_DIR build)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set KODI_MIRROR to the default if not already set
if(NOT DEFINED KODI_MIRROR)
  set(KODI_MIRROR "http://mirrors.kodi.tv")
endif()

message(STATUS "Mirror download location: ${KODI_MIRROR}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(WARNING "In-source build detected. It is recommended to build out-of-source.")
endif()

# Hide messages from Check*.cmake (e.g. CheckCSourceCompiles)
if(NOT VERBOSE)
  set(CMAKE_REQUIRED_QUIET ON)
  set(SEARCH_QUIET QUIET)
endif()

# Includes
include(cmake/modules/extra/ECMEnableSanitizers.cmake)
include(cmake/scripts/common/GeneratorSetup.cmake)
include(cmake/scripts/common/AddOptions.cmake)
include(cmake/scripts/common/Platform.cmake)
include(cmake/scripts/common/ArchSetup.cmake)
include(cmake/scripts/common/Macros.cmake)
include(cmake/scripts/common/ProjectMacros.cmake)
core_find_versions()
include(cmake/scripts/common/PathSetup.cmake)
include(cmake/scripts/common/CompilerSettings.cmake)
include(ExternalProject)
include(CMakeDependentOption)
include(cmake/scripts/common/DependencyOptions.cmake)

# general
option(VERBOSE            "Enable verbose output?" OFF)
option(ENABLE_CLANGTIDY   "Enable clang-tidy support?" OFF)
option(ENABLE_CPPCHECK    "Enable cppcheck support?" OFF)
option(ENABLE_DVDCSS      "Enable libdvdcss support?" ON)
option(ENABLE_INCLUDEWHATYOUUSE "Enable include-what-you-use support?" OFF)
option(ENABLE_UPNP        "Enable UPnP support?" ON)
option(ENABLE_AIRTUNES    "Enable AirTunes support?" ON)
option(ENABLE_OPTICAL     "Enable optical support?" ON)
option(ENABLE_PYTHON      "Enable python support?" ON)
option(ENABLE_TESTING     "Enable testing support?" ON)

# Internal Depends - supported on all platforms

# These are required enabled for all CI platforms, and recommended for all builds
option(ENABLE_INTERNAL_CROSSGUID "Enable internal crossguid?" ON)

# use ffmpeg from depends or system
option(ENABLE_INTERNAL_FFMPEG "Enable internal ffmpeg?" OFF)

# These are built for all platforms not using system libs or disabled by user
dependent_option(ENABLE_INTERNAL_ASS "Enable internal libass?")
dependent_option(ENABLE_INTERNAL_CEC "Enable internal libcec?")
dependent_option(ENABLE_INTERNAL_CURL "Enable internal libcurl?")
dependent_option(ENABLE_INTERNAL_EXIV2 "Enable internal exiv2?")
dependent_option(ENABLE_INTERNAL_FLATBUFFERS "Enable internal flatbuffers?")
dependent_option(ENABLE_INTERNAL_FMT "Enable internal fmt?")
dependent_option(ENABLE_INTERNAL_NFS "Enable internal libnfs?")
dependent_option(ENABLE_INTERNAL_PCRE2 "Enable internal pcre2?")
dependent_option(ENABLE_INTERNAL_NLOHMANNJSON "Enable internal nlohmannjson?")
dependent_option(ENABLE_INTERNAL_XSLT "Enable internal libxslt?")

# If ENABLE_INTERNAL_FMT is ON, we force ENABLE_INTERNAL_SPDLOG ON as it has a hard
# dependency on fmt
if(ENABLE_INTERNAL_FMT)
  option(ENABLE_INTERNAL_SPDLOG "Enable internal spdlog Forced" ON)
else()
  dependent_option(ENABLE_INTERNAL_SPDLOG "Enable internal spdlog?")
endif()

dependent_option(ENABLE_INTERNAL_TAGLIB "Enable internal taglib?")

if(KODI_DEPENDSBUILD OR WIN32 OR WINDOWS_STORE)
  dependent_option(ENABLE_INTERNAL_TINYXML2 "Enable internal TinyXML2?")
endif()

# Internal Depends - supported on UNIX platforms
if(UNIX)
  option(FFMPEG_PATH        "Path to external ffmpeg?" "")
  option(ENABLE_INTERNAL_FSTRCMP "Enable internal fstrcmp?" OFF)
  if(ENABLE_INTERNAL_FFMPEG)
    option(ENABLE_INTERNAL_DAV1D "Enable internal dav1d?" OFF)
  endif()
  option(ENABLE_INTERNAL_GTEST "Enable internal gtest?" OFF)
endif()

# System options
if(NOT WIN32)
  option(WITH_ARCH              "build with given arch" OFF)
  option(WITH_CPU               "build with given cpu" OFF)
endif()
if(CORE_SYSTEM_NAME STREQUAL linux)
  option(ENABLE_EVENTCLIENTS    "Enable event clients support?" OFF)
  option(ADDONS_CONFIGURE_AT_STARTUP "Configure binary addons at startup?" ON)
endif()

# Build static libraries per directory
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio" AND NOT CMAKE_GENERATOR STREQUAL Xcode)
  set(ENABLE_STATIC_LIBS TRUE)
else()
  set(ENABLE_STATIC_LIBS FALSE)
endif()

core_find_git_rev(APP_SCMID FULL)

set(AUDIO_BACKENDS_LIST "" CACHE STRING "Available audio backends")
set(GL_INTERFACES_LIST "" CACHE STRING "Available GL interfaces")

# Dynamically loaded libraries built with the project
add_custom_target(${APP_NAME_LC}-libraries)
set(LIBRARY_FILES "" CACHE STRING "" FORCE)
mark_as_advanced(LIBRARY_FILES)

set(INCLUDES ${CMAKE_SOURCE_DIR}
             ${CMAKE_SOURCE_DIR}/lib
             ${CMAKE_SOURCE_DIR}/xbmc
             ${CMAKE_SOURCE_DIR}/xbmc/${PLATFORM_DIR}
             ${CMAKE_SOURCE_DIR}/xbmc/cores/VideoPlayer
             ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
             ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)

if (NOT PLATFORMDEFS_DIR STREQUAL "")
  list(APPEND INCLUDES ${CMAKE_SOURCE_DIR}/xbmc/${PLATFORMDEFS_DIR})
endif()

find_package(PkgConfig ${SEARCH_QUIET})
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED ${SEARCH_QUIET})
list(APPEND DEPLIBS ${CMAKE_THREAD_LIBS_INIT})

# Clean any existing generated build dir cmake files
file(REMOVE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ExportFiles.cmake)
file(REMOVE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/GeneratedPackSkins.cmake)

foreach(depspec ${PLATFORM_REQUIRED_DEPS})
  # We need to specify ENABLE_${PLATFORM_REQUIRED_DEPS} in order for the
  # optional subdirectory for the specified platform to get built.
  split_dependency_specification(${depspec} dep version)
  string(TOUPPER ${dep} depup)
  if(NOT ${ENABLE_${depup}} STREQUAL AUTO AND NOT ${ENABLE_${depup}})
    message(WARNING "Your request to disable the dependency ${dep} required on platform ${CORE_PLATFORM_NAME} was ignored. Please choose another platform or add \"-DENABLE_${depup}=ON\" to your CMake command line to resolve this warning.")
  endif()
  set(ENABLE_${depup} "ON" CACHE BOOL "Force enabling required ${depup} support" FORCE)
endforeach()

# Required tools. Keep in alphabetical order please
set(required_buildtools FlatC
                        JsonSchemaBuilder
                        TexturePacker
                        ${PLATFORM_REQUIRED_TOOLS}
)

# Optional build tools
set(optional_buildtools CCache
                        ClangFormat
                        ClangTidy
                        Cppcheck
                        IncludeWhatYouUse)

# Find build tools - Optional and required
core_optional_dep(${optional_buildtools})
core_require_dep(${required_buildtools})

# We want to explicitly ignore NATIVEPREFIX for any find calls to build target dependencies
if(NATIVEPREFIX)
  set(CMAKE_IGNORE_PREFIX_PATH ${NATIVEPREFIX})
endif()

# Required dependencies. Keep in alphabetical order please
set(required_deps ASS>=0.15.0
                  CrossGUID
                  Curl
                  Exiv2
                  FFMPEG
                  FlatBuffers
                  Fmt
                  FreeType
                  FriBidi
                  Fstrcmp
                  HarfBuzz
                  Iconv
                  LibDvd
                  Lzo2
                  OpenSSL>=1.1.0
                  PCRE2
                  NlohmannJSON>=3.2.0
                  Spdlog
                  Sqlite3
                  TagLib>=1.9.0
                  TinyXML
                  TinyXML2
                  ZLIB
                  ${PLATFORM_REQUIRED_DEPS})

# Optional dependencies. Keep in alphabetical order please
set(optional_deps Alsa>=1.0.27
                  Avahi
                  Bluetooth
                  Bluray>=0.9.3
                  CAP
                  CEC>=4.0.0
                  Dav1d
                  DBus
                  Iso9660pp>=2.1.0
                  LCMS2>=2.10
                  LircClient
                  MDNS
                  MicroHttpd>=0.9.40
                  NFS>=3.0.0
                  Pipewire>=0.3.50
                  Plist
                  PulseAudio>=11.0.0
                  Python
                  SmbClient
                  Sndio
                  UDEV
                  Udfread
                  XSLT
                  ${PLATFORM_OPTIONAL_DEPS})

# Remove excluded platform specific optional_deps
foreach(excludedep ${PLATFORM_OPTIONAL_DEPS_EXCLUDE})
  list(FILTER optional_deps EXCLUDE REGEX ${excludedep})
endforeach()

# Check optional deps first, since their availability can influence required ones, e.g. ffmpeg
core_optional_dep(${optional_deps})
core_require_dep(${required_deps})

# Check for any libs to be packaged with main app. eg libaacs
# These libs are not directly linked with main app, but may be used if available from a 
# dependency in the main app. eg libbluray can dyload libaacs for additional features
if(PLATFORM_OPTIONAL_PACKAGES)
  core_optional_package_lib(${PLATFORM_OPTIONAL_PACKAGES})
endif()

if(ENABLE_MARIADBCLIENT AND NOT ENABLE_MARIADBCLIENT STREQUAL AUTO AND ENABLE_MYSQLCLIENT AND NOT ENABLE_MYSQLCLIENT STREQUAL AUTO)
  MESSAGE(FATAL_ERROR "You can not use MySql and MariaDB at the same time. Disable one by adding -DENABLE_MYSQLCLIENT=OFF or -DENABLE_MARIADBCLIENT=OFF.")
elseif(ENABLE_MYSQLCLIENT AND NOT ENABLE_MYSQLCLIENT STREQUAL AUTO)
  set(ENABLE_MARIADBCLIENT OFF CACHE BOOL "")
endif()

core_optional_dep(MariaDBClient)
if(NOT TARGET ${APP_NAME_LC}::MariaDBClient)
  core_optional_dep(MySqlClient)
endif()

if(NOT TARGET ${APP_NAME_LC}::UDEV)
  core_optional_dep(LibUSB)
endif()

if(ENABLE_UPNP)
  list(APPEND DEP_DEFINES "-DHAS_UPNP=1")
endif()

if(ENABLE_OPTICAL)
  core_require_dep(Cdio>=0.80)
  list(APPEND DEP_DEFINES -DHAS_OPTICAL_DRIVE -DHAS_CDDA_RIPPER)
endif()

if(ENABLE_AIRTUNES)
  find_package(Shairplay ${SEARCH_QUIET})
  if(TARGET ${APP_NAME_LC}::Shairplay)
    core_require_dep(Shairplay)
  endif()
endif()

# We unset this after the bulk of our find calls are complete. Ideally we would want this enabled
# for anything that is build target related, and only remove for native buildtools, however
# thats more complicated for the need right now.
if(NATIVEPREFIX)
  unset(CMAKE_IGNORE_PREFIX_PATH)
endif()

# find all folders containing addon.xml.in
# used to define ADDON_XML_OUTPUTS, ADDON_XML_DEPENDS and ADDON_INSTALL_DATA
# Function defined in ./cmake/scripts/common/Macros.cmake
set(outputFilterRegex "addons/xbmc.json")
find_addon_xml_in_files(${outputFilterRegex})

if(TARGET ${APP_NAME_LC}::Alsa AND TARGET ${APP_NAME_LC}::PulseAudio)
  list(APPEND AUDIO_BACKENDS_LIST "alsa+pulseaudio")
endif()

# Compile Info
add_custom_command(OUTPUT ${CORE_BUILD_DIR}/xbmc/CompileInfo.cpp
                   COMMAND ${CMAKE_COMMAND} -DCORE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                                            -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
                                            -DCORE_PLATFORM_NAME_LC="${CORE_PLATFORM_NAME_LC}"
                                            -DAUDIO_BACKENDS="${AUDIO_BACKENDS_LIST}"
                                            -DGL_INTERFACES="${GL_INTERFACES_LIST}"
                                            -DCORE_BUILD_DIR=${CORE_BUILD_DIR}
                                            -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                                            -DARCH_DEFINES="${ARCH_DEFINES}"
                                            -DAPP_SCMID=${APP_SCMID}
                                            -DAPP_COPYRIGHT_YEARS=${APP_COPYRIGHT_YEARS}
                                            -DAPP_BUILD_DATE=${APP_BUILD_DATE}
                                            -DAPP_SHARED_LIBRARY_SUFFIX="${APP_SHARED_LIBRARY_SUFFIX}"
                                            -DPYTHON_VERSION=${PYTHON_VERSION}
                                            -Dprefix=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                            -DKODI_WEBSERVER_EXTRA_WHITELIST="${KODI_WEBSERVER_EXTRA_WHITELIST}"
                                            -P ${CMAKE_SOURCE_DIR}/cmake/scripts/common/GenerateCompileInfo.cmake
                   DEPENDS ${CMAKE_SOURCE_DIR}/version.txt
                           ${CMAKE_SOURCE_DIR}/xbmc/CompileInfo.cpp.in)
list(APPEND install_data ${ADDON_INSTALL_DATA})
add_library(compileinfo OBJECT ${CORE_BUILD_DIR}/xbmc/CompileInfo.cpp)
set_target_properties(compileinfo PROPERTIES FOLDER "Build Utilities")
target_compile_options(compileinfo PRIVATE ${SYSTEM_DEFINES} ${ARCH_DEFINES})
target_link_libraries(compileinfo PRIVATE ${APP_NAME_LC}::Fmt)

if(NOT MSVC)
  target_compile_options(compileinfo PUBLIC ${CORE_COMPILE_OPTIONS})
endif()

# RC File
if(WIN32)
  configure_file(${CMAKE_SOURCE_DIR}/xbmc/platform/win32/XBMC_PC.rc.in
                 ${CORE_BUILD_DIR}/xbmc/platform/win32/XBMC_PC.rc @ONLY)
  add_library(resources OBJECT ${CORE_BUILD_DIR}/xbmc/platform/win32/XBMC_PC.rc)
  set_target_properties(resources PROPERTIES FOLDER "Build Utilities")
  target_include_directories(resources PRIVATE ${CMAKE_SOURCE_DIR}/tools/windows/packaging/media)
  set(RESOURCES $<TARGET_OBJECTS:resources>)
endif()

# Generate messages
core_add_subdirs_from_filelist(${CMAKE_SOURCE_DIR}/cmake/messages/flatbuffers/*.txt)

include_directories(${INCLUDES})
add_compile_options(${ARCH_DEFINES} "${SYSTEM_DEFINES}" ${DEP_DEFINES} ${PATH_DEFINES})

set(core_DEPENDS "" CACHE STRING "" FORCE)
set(test_archives "" CACHE STRING "" FORCE)
set(test_sources "" CACHE STRING "" FORCE)
mark_as_advanced(core_DEPENDS)
mark_as_advanced(test_archives)
mark_as_advanced(test_sources)

# copy files to build tree
copy_files_from_filelist_to_buildtree(${CMAKE_SOURCE_DIR}/cmake/installdata/common/*.txt
                                      ${CMAKE_SOURCE_DIR}/cmake/installdata/${CORE_SYSTEM_NAME}/*.txt)

list(APPEND SKINS "${CMAKE_SOURCE_DIR}/addons/skin.estuary\;${CMAKE_SOURCE_DIR}")

if(NOT ${CORE_SYSTEM_NAME} MATCHES "windows")
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/system/players/VideoPlayer)
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL android AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  configure_file(${CMAKE_SOURCE_DIR}/xbmc/DllPaths_generated.h.in
                 ${CORE_BUILD_DIR}/DllPaths_generated.h @ONLY)
elseif(CORE_SYSTEM_NAME STREQUAL android)
  configure_file(${CMAKE_SOURCE_DIR}/xbmc/DllPaths_generated_android.h.in
                 ${CORE_BUILD_DIR}/DllPaths_generated_android.h @ONLY)
endif()

# main library (used for main binary and tests)
if(CORE_SYSTEM_NAME STREQUAL "darwin_embedded")
  # $<TARGET_OBJECTS:> as at 3.26.4 provides incorrect paths for ios/tvos platforms
  # Even if XCODE_EMIT_EFFECTIVE_PLATFORM_NAME global property is used, the xcode project
  # still sets int dir paths to $(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME) and the generator
  # expression only provides a path using $(CONFIGURATION)
  add_library(compileinfo_obj OBJECT IMPORTED)
  set_property(TARGET compileinfo_obj PROPERTY IMPORTED_OBJECTS
    "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/compileinfo.build/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/$(OBJECT_FILE_DIR_normal:base)/$(CURRENT_ARCH)/CompileInfo.o"
  )
  add_library(lib${APP_NAME_LC} STATIC)
  add_dependencies(lib${APP_NAME_LC} compileinfo)
  target_link_libraries(lib${APP_NAME_LC} PUBLIC compileinfo_obj)
else()
  add_library(lib${APP_NAME_LC} STATIC $<TARGET_OBJECTS:compileinfo>)
endif()
set_target_properties(lib${APP_NAME_LC} PROPERTIES PREFIX "")

# Other files (IDE)
set(OTHER_FILES cmake/README.md)

# Subdirs
foreach(CORE_PLATFORM_LC ${CORE_PLATFORM_NAME_LC})
  core_add_subdirs_from_filelist(${CMAKE_SOURCE_DIR}/cmake/treedata/common/${CORE_PLATFORM_LC}/*.txt
                                 ${CMAKE_SOURCE_DIR}/cmake/treedata/${CORE_SYSTEM_NAME}/${CORE_PLATFORM_LC}/*.txt)
endforeach()

core_add_subdirs_from_filelist(${CMAKE_SOURCE_DIR}/cmake/treedata/common/*.txt
                               ${CMAKE_SOURCE_DIR}/cmake/treedata/${CORE_SYSTEM_NAME}/*.txt)
core_add_optional_subdirs_from_filelist(${CMAKE_SOURCE_DIR}/cmake/treedata/optional/common/*.txt
                                        ${CMAKE_SOURCE_DIR}/cmake/treedata/optional/${CORE_SYSTEM_NAME}/*.txt)

# core_DEPENDS: these are the artifact/object files of /xbmc/*
# SYSTEM_LDFLAGS: system linker flags
# DEPLIBS: system libraries for linking
target_link_libraries(lib${APP_NAME_LC} PUBLIC ${core_DEPENDS} ${SYSTEM_LDFLAGS} ${DEPLIBS} ${CMAKE_DL_LIBS})
core_target_link_libraries(lib${APP_NAME_LC})
set_target_properties(lib${APP_NAME_LC} PROPERTIES PROJECT_LABEL "xbmc")
source_group_by_folder(lib${APP_NAME_LC} RELATIVE ${CMAKE_SOURCE_DIR}/xbmc)
if(WIN32)
  add_precompiled_header(lib${APP_NAME_LC} pch.h ${CMAKE_SOURCE_DIR}/xbmc/platform/win32/pch.cpp)
endif()

# main binary
if(NOT CORE_SYSTEM_NAME STREQUAL android)
  if(CORE_SYSTEM_NAME STREQUAL windowsstore)
    winstore_add_target_properties(${APP_NAME_LC})
  endif()
  add_executable(${APP_NAME_LC} ${CORE_MAIN_SOURCE} "${RESOURCES}" ${OTHER_FILES})
  set_target_properties(${APP_NAME_LC} PROPERTIES ENABLE_EXPORTS ON)
else()
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
  add_library(${APP_NAME_LC} SHARED ${CORE_MAIN_SOURCE} "${RESOURCES}" ${OTHER_FILES})

  # Statically resolve global references to shared library (ie. ffmpeg) definitions.
  # Related to https://stackoverflow.com/questions/46307266/including-objects-to-a-shared-library-from-a-c-archive-a
  set_target_properties(${APP_NAME_LC} PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic")
endif()
add_dependencies(${APP_NAME_LC} ${APP_NAME_LC}-libraries)

whole_archive(_MAIN_LIBRARIES ${core_DEPENDS})
target_link_libraries(${APP_NAME_LC} ${_MAIN_LIBRARIES} lib${APP_NAME_LC} ${DEPLIBS})
unset(_MAIN_LIBRARIES)

if(${CORE_SYSTEM_NAME} MATCHES "windows")
  set(_bundle_dir $<TARGET_FILE_DIR:${APP_NAME_LC}>)
else()
  set(_bundle_dir ${CMAKE_BINARY_DIR})
endif()

# These are skins that are copied into place from the source tree
set(XBT_SOURCE_FILELIST "")
foreach(skin ${SKINS})
  list(GET skin 0 dir)
  list(GET skin 1 relative)
  copy_skin_to_buildtree(${dir} ${relative})
endforeach()

# Generate system addons
add_custom_target(gen_system_addons
                    COMMAND ${CMAKE_COMMAND} -DCORE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                                             -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
                                             -DCORE_PLATFORM_NAME_LC="${CORE_PLATFORM_NAME_LC}"
                                             -DCORE_BUILD_DIR=${CORE_BUILD_DIR}
                                             -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                                             -DBUNDLEDIR=${_bundle_dir}
                                             -P ${CMAKE_SOURCE_DIR}/cmake/scripts/common/GenerateSystemAddons.cmake
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if(_multiconfig_generator)
  # Generator expressions in add_custom_command(OUTPUT) are only available in cmake 3.20+
  # we dont really need config aware locations for a single config generator, so we only
  # set this for multi config generators who all use newer cmake
  set(CONFIG_VAR $<CONFIG>)
endif()

# Pack skins and copy to correct build dir (MultiConfig Generator aware)
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${CONFIG_VAR}/gen_skin.timestamp
  COMMAND ${CMAKE_COMMAND} -DBUNDLEDIR=${_bundle_dir}
                           -DTEXTUREPACKER_EXECUTABLE=$<TARGET_FILE:TexturePacker::TexturePacker::Executable>
                           -P ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/GeneratedPackSkins.cmake
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${CONFIG_VAR}
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${CONFIG_VAR}/gen_skin.timestamp
  DEPENDS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/GeneratedPackSkins.cmake
          ${XBT_SOURCE_FILELIST}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Generating skin xbt"
)
add_custom_target(gen_skin_pack DEPENDS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${CONFIG_VAR}/gen_skin.timestamp)

# Packaging target. This generates system addon, xbt creation, copy files to build tree
add_custom_target(generate-packaging ALL
                  DEPENDS TexturePacker::TexturePacker::Executable export-files gen_skin_pack gen_system_addons)

core_target_add_dependencies(export-files)

# optional package_libs must be built/available before exporting files to buildtrees
# some platforms require the libraries to exist, otherwise their packaging may not copy
# the requested package_libs into the final app
if(package_libs)
  add_dependencies(export-files ${package_libs})
endif()

# Add to lib${APP_NAME_LC} solely for Win UWP. msix building doesnt seem to pick up the
# generated buildtree if we do it later. Other platforms dont care when this happens.
add_dependencies(lib${APP_NAME_LC} generate-packaging)

set_target_properties(generate-packaging PROPERTIES FOLDER "Build Utilities")

if(WIN32)
  set_target_properties(${APP_NAME_LC} PROPERTIES WIN32_EXECUTABLE ON)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${APP_NAME_LC})
  if(NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
    target_sources(${APP_NAME_LC} PRIVATE ${CMAKE_SOURCE_DIR}/xbmc/platform/win32/app.manifest)
  else()
    set_target_properties(${APP_NAME_LC} PROPERTIES VS_USER_PROPS ${VCPROJECT_PROPS_FILE}
                          VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION ${VS_MINIMUM_SDK_VERSION})
  endif()
elseif(CORE_SYSTEM_NAME STREQUAL android)
  # Nothing
elseif(CORE_SYSTEM_NAME STREQUAL darwin_embedded OR CORE_SYSTEM_NAME STREQUAL osx)
  # Build all apple app target as bundled and dont sign.
  # We do all signing post build of the app target manually
  set_target_properties(${APP_NAME_LC} PROPERTIES OUTPUT_NAME ${APP_NAME}
                                                  MACOSX_BUNDLE YES
                                                  MACOSX_BUNDLE_INFO_PLIST ${PLATFORM_BUNDLE_INFO_PLIST}
                                                  XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
                                                  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
else()
  set_target_properties(${APP_NAME_LC} PROPERTIES SUFFIX ${APP_BINARY_SUFFIX})
endif()

# testing
if(HOST_CAN_EXECUTE_TARGET AND ENABLE_TESTING)
  find_package(Gtest 1.10.0 REQUIRED ${SEARCH_QUIET})

  copy_files_from_filelist_to_buildtree(${CMAKE_SOURCE_DIR}/cmake/installdata/test-reference-data.txt NO_INSTALL)
  add_executable(${APP_NAME_LC}-test EXCLUDE_FROM_ALL ${CMAKE_SOURCE_DIR}/xbmc/test/xbmc-test.cpp ${test_sources})

  set_target_properties(PROPERTIES ENABLE_EXPORTS ON)

  whole_archive(_TEST_LIBRARIES ${core_DEPENDS} ${GTEST_LIBRARY})
  target_link_libraries(${APP_NAME_LC}-test PRIVATE ${SYSTEM_LDFLAGS} ${_TEST_LIBRARIES} lib${APP_NAME_LC} ${DEPLIBS} ${CMAKE_DL_LIBS})
  unset(_TEST_LIBRARIES)

  if (ENABLE_INTERNAL_GTEST)
    add_dependencies(${APP_NAME_LC}-test ${APP_NAME_LC}-libraries generate-packaging gtest)
  endif()

  # Enable unit-test related targets
  enable_testing()
  gtest_add_tests(${APP_NAME_LC}-test "" ${test_sources})
  add_custom_target(check ${CMAKE_CTEST_COMMAND} WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  add_dependencies(check ${APP_NAME_LC}-test)

  # Valgrind (memcheck)
  find_program(VALGRIND_EXECUTABLE NAMES valgrind)
  if(VALGRIND_EXECUTABLE)
    set(CTEST_MEMORYCHECK_COMMAND ${VALGRIND_EXECUTABLE})
    set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "-q --trace-children=yes --leak-check=yes --track-origins=yes")
    include(CTest)
    add_custom_target(check-valgrind ${CMAKE_CTEST_COMMAND} -D ExperimentalMemCheck \${ARGS} WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    add_dependencies(check-valgrind ${APP_NAME_LC}-test)
  endif()

  # For testing commit series
  add_custom_target(check-commits ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/scripts/common/CheckCommits.cmake
                                                   -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
  set_target_properties(check-commits PROPERTIES FOLDER "Build Utilities")

  include(${CMAKE_SOURCE_DIR}/cmake/scripts/common/StaticAnalysis.cmake)

  # code coverage
  if(CMAKE_BUILD_TYPE STREQUAL Coverage)
    if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/CodeCoverage.cmake)
      include(${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/CodeCoverage.cmake)
    else()
      message(FATAL_ERROR "Code coverage not (yet) implemented for platform ${CORE_SYSTEM_NAME}")
    endif()
  endif()
endif()

# Documentation
find_package(Doxygen ${SEARCH_QUIET})
if(DOXYGEN_FOUND)
  add_custom_target(doc
                    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/doxygen/Doxyfile.doxy
                    COMMAND ${CMAKE_COMMAND} -E echo "Documentation built to: file://${CMAKE_SOURCE_DIR}/docs/html/index.html"
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/doxygen
                    COMMENT "Generating Doxygen documentation" VERBATIM)
endif()

# link wrapper
if(FFMPEG_LINK_EXECUTABLE)
  set(CMAKE_CXX_LINK_EXECUTABLE "${FFMPEG_LINK_EXECUTABLE}")
endif()
if(FFMPEG_CREATE_SHARED_LIBRARY)
  set(CMAKE_CXX_CREATE_SHARED_LIBRARY "${FFMPEG_CREATE_SHARED_LIBRARY}")
endif()

# Platform specific additional extra targets
if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/ExtraTargets.cmake)
  include(${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/ExtraTargets.cmake)
endif()

include(cmake/scripts/${CORE_SYSTEM_NAME}/Install.cmake)

# Add uninstall target
if(CMAKE_GENERATOR MATCHES Makefile)
  add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/scripts/common/Uninstall.cmake)
  set_target_properties(uninstall PROPERTIES FOLDER "Build Utilities")
endif()

# Create target that allows to build binary-addons.
# Use make binary-addons ADDONS="visualization.spectrum" to select the addons to build.
if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  if(CMAKE_CROSSCOMPILING)
    set(_cross_args CROSS_COMPILING=yes)
  endif()
  if(NOT CORE_SYSTEM_NAME STREQUAL android)
    set(_prefix ${CMAKE_BINARY_DIR}/addons)
    set(_extra "CMAKE_EXTRA=-DPACKAGE_ZIP=ON")
  else()
    set(_prefix ${CMAKE_INSTALL_PREFIX})
  endif()
  add_custom_target(binary-addons
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/tools/depends/target/binary-addons clean
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/tools/depends/target/binary-addons VERBOSE=1 V=99
                    PREFIX=${_prefix} INSTALL_PREFIX=${_prefix} ${_cross_args} ${_extra})
  unset(_cross_args)
  unset(_prefix)
endif()

# Prepare add-on build env
include(${CORE_SOURCE_DIR}/xbmc/addons/AddonBindings.cmake)
file(COPY ${CORE_ADDON_BINDINGS_FILES} ${CORE_ADDON_BINDINGS_DIRS}/
     DESTINATION ${CORE_BUILD_DIR}/include/${APP_NAME_LC}
     REGEX ".txt" EXCLUDE)

set(APP_LIB_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/${APP_NAME_LC})
set(APP_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/${APP_NAME_LC})
configure_file(${CMAKE_SOURCE_DIR}/cmake/KodiConfig.cmake.in
               ${CORE_BUILD_DIR}/lib/${APP_NAME_LC}/${APP_NAME}Config.cmake @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/common/AddonHelpers.cmake
               ${CORE_BUILD_DIR}/lib/${APP_NAME_LC}/AddonHelpers.cmake COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/common/AddOptions.cmake
               ${CORE_BUILD_DIR}/lib/${APP_NAME_LC}/AddOptions.cmake COPYONLY)

# Status
message(STATUS "#---- CONFIGURATION ----#")
foreach(msg ${final_message})
  message(STATUS ${msg})
endforeach()

if(VERBOSE)
  message(STATUS "\n#--------------- Internal Variables -------------#")
  message(STATUS "DEPLIBS: ${DEPLIBS}")
  message(STATUS "core_DEPENDS: ${core_DEPENDS}")
  message(STATUS "#---------------------------------------------#")
  message(STATUS "SYSTEM_LDFLAGS        : ${SYSTEM_LDFLAGS}")
  message(STATUS "CMAKE_DL_LIBS         : ${CMAKE_DL_LIBS}")
  message(STATUS "CMAKE_C_FLAGS         : ${CMAKE_C_FLAGS}")
  message(STATUS "CMAKE_CXX_FLAGS       : ${CMAKE_CXX_FLAGS}")
  message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
  message(STATUS "LTO_OPTIMIZATION      : ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
  if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(STATUS "CMAKE_CXX_COMPILE_OPTIONS_IPO: ${CMAKE_CXX_COMPILE_OPTIONS_IPO}")
    message(STATUS "CMAKE_C_COMPILE_OPTIONS_IPO  : ${CMAKE_C_COMPILE_OPTIONS_IPO}")
    message(STATUS "CMAKE_AR                     : ${CMAKE_AR}")
    message(STATUS "CMAKE_NM                     : ${CMAKE_NM}")
    message(STATUS "CMAKE_LINKER                 : ${CMAKE_LINKER}")
  endif()
  message(STATUS "#---------------------------------------------#")
  message(STATUS "bindir     : ${bindir}")
  message(STATUS "includedir : ${includedir}")
  message(STATUS "libdir     : ${libdir}")
  message(STATUS "datarootdir: ${datarootdir}")
  message(STATUS "#---------------------------------------------#")
  message(STATUS "BINARY: ${APP_NAME_LC}${APP_BINARY_SUFFIX}")
  message(STATUS "#---------------------------------------------#")
  message(STATUS "GIT_REV: ${APP_SCMID}")
  message(STATUS "Build date: ${APP_BUILD_DATE}")
  message(STATUS "#---------------------------------------------#")
  message(STATUS "CPACK_GENERATOR       : ${CPACK_GENERATOR}")
  message(STATUS "CPACK_SOURCE_GENERATOR: ${CPACK_SOURCE_GENERATOR}")
  message(STATUS "#---------------------------------------------#")
endif()
