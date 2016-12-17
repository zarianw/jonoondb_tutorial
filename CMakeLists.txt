cmake_minimum_required (VERSION 2.8)

project (JonoonDB_Tutorial)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/schemas)

if(APPLE)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++ -m64")
elseif(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++14 -m64")
endif()

# jonoondb
if(JONOONDB_ROOT)
  message(JONOONDB_ROOT = ${JONOONDB_ROOT})
else()
  message("JONOONDB_ROOT is not set. Standard system locations will be used to lookup headers and libs.")
endif()


include_directories(${JONOONDB_ROOT}/include)
find_library (JonoonDB_LIBRARY jonoondb_api_v0 ${JONOONDB_ROOT}/lib)

set (DB_PATH ${PROJECT_BINARY_DIR})
set (SCHEMA_PATH ${PROJECT_SOURCE_DIR}/schemas)
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_SOURCE_DIR}/config_generated.h")

# jonoondb_tutorial
add_executable(
 jonoondb_tutorial
 ${PROJECT_SOURCE_DIR}/main.cc)
target_link_libraries(jonoondb_tutorial ${JonoonDB_LIBRARY})
