#!/usr/bin/env bash
# mayhem/build.sh — build the mpc-qt fuzz harnesses and the known-answer selftest.
# Upstream builds one GUI executable (mpc-qt) with no library target; the harnesses compile the
# src/*.cpp files that parse untrusted input (helpers, playlist, storage + the logger/platform code
# they need) via mayhem/fuzz/CMakeLists.txt, against the distro Qt6. Re-runnable and offline:
# every dependency is an apt package baked into the image.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS SRC

cd "$SRC"

# 1) Fuzz build: the project sources with ASan+UBSan + SanCov (fuzzer-no-link) and DWARF 3.
# mayhem/lsan_off.cc (__lsan_is_turned_off) is linked into every fuzz and -standalone binary.
cmake -S mayhem/fuzz -B mayhem/build-fuzz -G Ninja -DCMAKE_BUILD_TYPE=None -DMPCQT_FUZZ=ON \
    -DLSAN_OFF_SRC="$SRC/mayhem/lsan_off.cc" \
    -DCMAKE_C_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS" \
    -DCMAKE_CXX_FLAGS="-fsanitize=fuzzer-no-link $SANITIZER_FLAGS $DEBUG_FLAGS -O1"
cmake --build mayhem/build-fuzz -j"$MAYHEM_JOBS"
for t in timeparse format playlist config http; do
    cp -f "mayhem/build-fuzz/fuzz_$t" "/mayhem/fuzz_$t"
    cp -f "mayhem/build-fuzz/fuzz_$t-standalone" "/mayhem/fuzz_$t-standalone"
done

# 2) Selftest with normal flags (the functional oracle run by mayhem/test.sh).
cmake -S mayhem/fuzz -B mayhem/build-test -G Ninja -DCMAKE_BUILD_TYPE=Release -DMPCQT_FUZZ=OFF \
    -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" -DCMAKE_EXE_LINKER_FLAGS="$COVERAGE_FLAGS"
cmake --build mayhem/build-test -j"$MAYHEM_JOBS"
