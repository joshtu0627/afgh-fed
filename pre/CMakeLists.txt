cmake_minimum_required(VERSION 2.8)
project(relic_hom)

INCLUDE_DIRECTORIES(/usr/local/include)

find_library(RELIC_LOC relic PATHS /usr/local/lib)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -std=c++11 -o2" )
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -g -Wall -march=native -Wl,-rpath=/usr/local/lib -o2")
#
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin )
##set(SOURCE_FILES pre-afgh-relic.h keygen.c encryption.c encoding.c utils.c pre-hom.c)
#set(SOURCE_FILES pre-afgh-relic.h pre-hom.c pre-hom.h)
#
#add_library(pre-afgh-relic SHARED ${SOURCE_FILES})
#
#add_executable(test_pre test.cpp ${SOURCE_FILES})
##add_executable(benchmark_pre benchmark.cpp ${SOURCE_FILES})
#
#target_link_libraries(test_pre ${RELIC_LOC})
##target_link_libraries(benchmark_pre ${RELIC_LOC})

#
set(RELIC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/gmp-pbc-128/librelic.so)
set(RELIC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/gmp-pbc-128-ss/librelic.so)
set(RELIC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libs/gmp-pbc-80/librelic.so)
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fabi-version=2 -fno-omit-frame-pointer -std=c++11 -O2" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fabi-version=2 -fno-omit-frame-pointer -g -Wall -march=native -Wl,-rpath=/usr/local/lib -O2")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin )

#set(SOURCE_FILES main.cpp pre-hom.c pre-hom.h pre-relic-udf.c pre-relic-udf.h)
set(SOURCE_FILES main.cpp pre-hom.c pre-hom.h pre-relic-udf.c test_pre_udf.h)
add_executable(relic_hom ${SOURCE_FILES})

#add_library(relic-proxy-re-enc SHARED proxy-re-wrapper.c pre-hom.c pre-hom.h)
add_library(relic-proxy-re-enc SHARED pre-hom.c pre-hom.h pre-relic-udf.c)

target_link_libraries(relic_hom ${RELIC_LOC})
