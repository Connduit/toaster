cmake_minimum_required(VERSION 3.10)
project(rtlsdr_fm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(RTLSDR REQUIRED librtlsdr)

# Sources are organized by module under src/ (core, rf, dsp/filters, audio,
# console) rather than split by file type into include/ vs src/ -- see
# README.md's "Project structure" section for why. Headers live alongside
# their .cpp files in the same module folder; every #include is written
# as a path relative to src/ (e.g. "dsp/filters/Filter.h"), which is why
# target_include_directories below points at src itself rather than at
# each module subfolder individually.
add_executable(rtlsdr_fm
    src/main.cpp
    src/core/Toaster.cpp
    src/core/Controller.cpp
    src/core/StageSelection.cpp
    src/messaging/Dispatcher.cpp
    src/rf/Receiver.cpp
    src/rf/RtlSdrSource.cpp
    src/audio/AudioStage.cpp
    src/audio/WavFileWriter.cpp
    src/spectrum/SpectrumStage.cpp
    src/spectrum/TerminalSpectrumRenderer.cpp
    src/spectrum/BarSpectrumRenderer.cpp
    src/spectrum/WaterfallRenderer.cpp
    src/dsp/Fft.cpp
    src/dsp/DigitalDownConverter.cpp
    src/spectrum/SpectrumCommands.cpp
    src/classification/DeviceCatalog.cpp
    src/classification/TerminalClassificationSink.cpp
    src/classification/ClassificationStage.cpp
    src/scanner/TerminalScannerSink.cpp
    src/scanner/ScannerStage.cpp
    src/dsp/filters/FirFilter.cpp
    src/dsp/filters/IirFilter.cpp
    src/dsp/filters/FirDesign.cpp
    src/dsp/filters/IirDesign.cpp
)

target_include_directories(rtlsdr_fm PRIVATE
    src
    ${RTLSDR_INCLUDE_DIRS}
)
target_link_libraries(rtlsdr_fm PRIVATE ${RTLSDR_LIBRARIES} pthread)
target_compile_options(rtlsdr_fm PRIVATE ${RTLSDR_CFLAGS_OTHER})
