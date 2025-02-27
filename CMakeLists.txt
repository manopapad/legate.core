#=============================================================================
# Copyright 2022 NVIDIA Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================

cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)

if(POLICY CMP0060)
  # Link libraries by full path even in implicit directories
  # https://cmake.org/cmake/help/latest/policy/CMP0060.html#policy:CMP0060
  cmake_policy(SET CMP0060 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0060 NEW)
endif()

if(POLICY CMP0077)
  # option() honors normal variables
  # https://cmake.org/cmake/help/latest/policy/CMP0077.html
  cmake_policy(SET CMP0077 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
endif()

if(POLICY CMP0096)
  # The project() command preserves leading zeros in version components
  # https://cmake.org/cmake/help/latest/policy/CMP0096.html
  cmake_policy(SET CMP0096 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0096 NEW)
endif()

if(POLICY CMP0126)
  # make set(CACHE) command not remove normal variable of the same name from the current scope
  # https://cmake.org/cmake/help/latest/policy/CMP0126.html
  cmake_policy(SET CMP0126 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0126 NEW)
endif()

if(POLICY CMP0135)
  # make the timestamps of ExternalProject_ADD match the download time
  # https://cmake.org/cmake/help/latest/policy/CMP0135.html
  cmake_policy(SET CMP0135 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

##############################################################################
# - Download and initialize RAPIDS CMake helpers -----------------------------

if(NOT EXISTS ${CMAKE_BINARY_DIR}/RAPIDS.cmake)
  file(DOWNLOAD https://raw.githubusercontent.com/rapidsai/rapids-cmake/branch-22.08/RAPIDS.cmake
       ${CMAKE_BINARY_DIR}/RAPIDS.cmake)
endif()
include(${CMAKE_BINARY_DIR}/RAPIDS.cmake)
include(rapids-cmake)
include(rapids-cpm)
include(rapids-cuda)
include(rapids-export)
include(rapids-find)

set(legate_core_version 22.10.00)

# For now we want the optimization flags to match on both normal make and cmake
# builds so we override the cmake defaults here for release, this changes
# -O3 to -O2 and removes -DNDEBUG
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CUDA_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CUDA_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CUDA_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_CUDA_FLAGS_RELWITHDEBINFO "-O2 -g")

if(NOT SKBUILD)
  project(legate_core VERSION ${legate_core_version} LANGUAGES C CXX)
  include(${CMAKE_CURRENT_SOURCE_DIR}/legate_core_cpp.cmake)
else()
  project(
    legate_core_python
    VERSION ${legate_core_version}
    LANGUAGES # TODO: Building Python extension modules via the python_extension_module requires the C
              # language to be enabled here. The test project that is built in scikit-build to verify
              # various linking options for the python library is hardcoded to build with C, so until
              # that is fixed we need to keep C.
              C CXX)
  include(${CMAKE_CURRENT_SOURCE_DIR}/legate_core_python.cmake)
endif()

if(CMAKE_GENERATOR STREQUAL "Ninja")
  function(add_touch_legate_core_ninja_build_target)
    set(_suf )
    set(_depends )
    if(SKBUILD)
      set(_suf "_python")
    endif()
    foreach(_dep IN ITEMS legion_core legion_core_python
                          Legion LegionRuntime
                          Realm RealmRuntime
                          Regent)
      if(TARGET ${_dep})
        list(APPEND _depends ${_dep})
      endif()
    endforeach()
    add_custom_target("touch_legion_core${_suf}_ninja_build" ALL
      COMMAND ${CMAKE_COMMAND} -E touch_nocreate "${CMAKE_CURRENT_BINARY_DIR}/build.ninja"
      COMMENT "touch build.ninja so ninja doesn't re-run CMake on rebuild"
      VERBATIM DEPENDS ${_depends}
    )
  endfunction()
  add_touch_legate_core_ninja_build_target()
endif()
