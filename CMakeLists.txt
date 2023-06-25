cmake_minimum_required(VERSION 3.25)
project(main CXX)

find_package(unofficial-angle REQUIRED CONFIG)

add_executable(main main.cpp)

#TODO : Find a better way to auto add Angle
find_path(VCPKG_INCLUDE_DIR zlib.h) #Angle needs zlib
target_include_directories(main PRIVATE ${VCPKG_INCLUDE_DIR})

target_link_libraries(main PRIVATE unofficial::angle::libGLESv2)
target_link_libraries(main PRIVATE unofficial::angle::libEGL)

target_compile_features(main PRIVATE cxx_std_20)
