// Shared setup for the mpc-qt fuzz harnesses: one QCoreApplication for the process
// (Qt string/JSON/translation code expects an application instance). No GUI is created.
#pragma once
#include <QCoreApplication>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

// src/logger.cpp has a static object (StdFileCopy realStdErr(stderr)) whose constructor dup()s
// stderr for its own use and then fclose()s stderr, closing fd 2. In the GUI app that is
// intended; in a fuzzer it silences libFuzzer's and the sanitizers' reports and lets the next
// open() land on fd 2. So: dup fd 2 before any C++ static initializer runs (priority 101), and
// put it back (fd 2 + the stderr FILE) once static init is done, from LLVMFuzzerInitialize.
static int mpcqt_saved_stderr = -1;
__attribute__((constructor(101))) static void mpcqt_save_stderr() {
    mpcqt_saved_stderr = dup(2);
}
static inline void mpcqt_restore_stderr() {
    if (mpcqt_saved_stderr < 0) return;
    dup2(mpcqt_saved_stderr, 2);
    freopen("/proc/self/fd/2", "a", stderr);
    setvbuf(stderr, nullptr, _IONBF, 0);
}

static inline void mpcqt_fuzz_init() {
    static int argc = 1;
    static char arg0[] = "mpc-qt-fuzz";
    static char *argv[] = {arg0, nullptr};
    static QCoreApplication *app = nullptr;
    if (!app) {
        mpcqt_restore_stderr();
        setenv("QT_QPA_PLATFORM", "offscreen", 1);
        app = new QCoreApplication(argc, argv);
    }
}
