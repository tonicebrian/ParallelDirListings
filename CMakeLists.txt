project(parallelListingDirs)

cmake_minimum_required(VERSION 2.8)

find_package(Boost COMPONENTS filesystem REQUIRED)

add_executable(dirList main.cpp)

target_link_libraries(dirList ${Boost_FILESYSTEM_LIBRARY}
                              pthread)
