# Builds the mpc-qt sources that parse untrusted input (helpers, playlist, storage and the
# logger/platform code they pull in) as a static library, plus the fuzz harnesses and the
# known-answer selftest. Upstream builds a single GUI executable with no library target, so
# this compiles the same src/*.cpp files directly. Flags come from CMAKE_CXX_FLAGS (build.sh).
cmake_minimum_required(VERSION 3.19)
project(mpcqt_fuzz CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
option(MPCQT_FUZZ "build the fuzz harnesses (else the selftest)" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Network DBus)
set(S ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

add_library(mpcqt_core STATIC
    ${S}/helpers.cpp ${S}/helpers.h
    ${S}/logger.cpp ${S}/logger.h
    ${S}/playlist.cpp ${S}/playlist.h
    ${S}/storage.cpp ${S}/storage.h
    ${S}/platform/unify.cpp ${S}/platform/unify.h
    ${S}/platform/devicemanager.cpp ${S}/platform/devicemanager.h
    ${S}/platform/devicemanager_unix.cpp ${S}/platform/devicemanager_unix.h
    ${S}/platform/screensaver.cpp ${S}/platform/screensaver.h
    ${S}/platform/screensaver_unix.cpp ${S}/platform/screensaver_unix.h
)
target_include_directories(mpcqt_core PUBLIC ${S} ${S}/platform ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mpcqt_core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets
    Qt6::Network Qt6::DBus)

# The web interface (ipc/http.cpp) includes manager.h -> mpvwidget.h -> libmpv headers.
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)
set(VERSTR "fuzz")
configure_file(${S}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
add_library(mpcqt_http STATIC ${S}/ipc/http.cpp ${S}/ipc/http.h ${S}/about.cpp ${S}/about.h)
target_include_directories(mpcqt_http PUBLIC ${MPV_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mpcqt_http PUBLIC mpcqt_core)

if(MPCQT_FUZZ)
    # OBJECT, not STATIC: nothing references the hook, so an archive member would never be linked.
    add_library(lsan_off OBJECT ${LSAN_OFF_SRC})
    add_library(standalone_main STATIC $ENV{STANDALONE_FUZZ_MAIN})
    set_source_files_properties($ENV{STANDALONE_FUZZ_MAIN} PROPERTIES LANGUAGE C)
    foreach(t timeparse format playlist config http)
        add_executable(fuzz_${t} fuzz_${t}.cpp)
        target_link_libraries(fuzz_${t} PRIVATE mpcqt_core mpcqt_http lsan_off)
        target_link_options(fuzz_${t} PRIVATE $ENV{LIB_FUZZING_ENGINE})
        target_compile_options(fuzz_${t} PRIVATE $ENV{LIB_FUZZING_ENGINE})
        add_executable(fuzz_${t}-standalone fuzz_${t}.cpp)
        target_link_libraries(fuzz_${t}-standalone PRIVATE mpcqt_core mpcqt_http lsan_off standalone_main)
    endforeach()
else()
    add_executable(mpcqt_selftest selftest.cpp)
    target_link_libraries(mpcqt_selftest PRIVATE mpcqt_core)
endif()
