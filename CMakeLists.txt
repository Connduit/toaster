cmake_minimum_required(VERSION 3.20)
project(toaster LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTLSDR REQUIRED IMPORTED_TARGET librtlsdr)


add_library(toaster_lib
    src/Decimator.cpp
    src/Dispatcher.cpp
    src/FIRDesign.cpp
    src/IIRDesign.cpp
    src/Receiver.cpp
    src/Toaster.cpp
    src/WavSink.cpp
    src/Demodulator.cpp
    src/DspProcessor.cpp
    src/FIRFilter.cpp
    src/IIRFilter.cpp
    src/RtlSdrDevice.cpp
    src/ToasterSubsystem.cpp
    src/Factory.cpp
)

target_include_directories(toaster_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include

    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(toaster_lib
    PRIVATE
        Threads::Threads
        PkgConfig::RTLSDR
)

add_executable(toaster app/main.cpp)

target_link_libraries(toaster PRIVATE toaster_lib)