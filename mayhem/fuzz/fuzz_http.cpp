// The MPC-HC compatible web interface (src/ipc/http.cpp): HttpServer request parsing
// (request line, headers, ?query and POST form splitting, HttpServer::urlDecode), the
// MpcHcServer routes (command.html wm_command dispatch, variables.html, browser.html,
// info.html, status.html, ...) and the response writer. The input is the raw bytes a client
// sends; it is delivered over a real loopback TCP connection to the server the app runs.
#include <unistd.h>
#include <QTcpSocket>
#include <QHostAddress>
#include "ipc/http.h"
#include "fuzz_common.h"

static MpcHcServer *server = nullptr;
static quint16 port = 0;

static bool tryConnect(QTcpSocket &c) {
    c.connectToHost(QHostAddress::LocalHost, port);
    while (c.state() == QAbstractSocket::HostLookupState ||
           c.state() == QAbstractSocket::ConnectingState)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    return c.state() == QAbstractSocket::ConnectedState;
}

// A fresh server per input: each request then sees the startup state, and the server-side
// sockets (children of the HttpServer, close()d but never deleted by it) are freed with it.
static MpcHcServer *makeServer(quint16 p) {
    auto *s = new MpcHcServer();
    s->setLocalHostOnly(true);
    // Player state the app pushes into the server from PlaybackManager/MainWindow signals at
    // startup (playbackState, rate, volume and mute are not initialized by MpcHcServer itself).
    s->setPlaybackState(PlaybackManager::StoppedState, false);
    s->setPlaybackRate(1.0);
    s->setVolume(100);
    s->setVolumeMuted(false);
    s->setFileTime(0.0, 0.0);
    s->setFileSize(0);
    s->setMediaTitle(QStringLiteral("Movie"));
    s->setNowPlaying(QUrl::fromLocalFile(QStringLiteral("/tmp/movie.mkv")));
    s->setTcpPort(p);
    s->setEnabled(true);
    return s;
}

extern "C" int LLVMFuzzerInitialize(int *, char ***) {
    mpcqt_fuzz_init();
    // The server's port is fixed by the app's settings (default 13579); pick a per-process port
    // so parallel fuzzer processes don't collide, and move on if it is already taken.
    for (int i = 0; i < 200 && !port; i++) {
        quint16 p = static_cast<quint16>(20000 + (getpid() * 7 + i * 131) % 40000);
        server = makeServer(p);
        QTcpSocket probe;
        port = p;
        if (!tryConnect(probe)) port = 0;
        probe.abort();
        delete server;
        server = nullptr;
    }
    if (!port) abort();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // The server only acts on a readyRead, which needs at least one byte of request.
    if (size == 0) return -1;
    server = makeServer(port);
    QTcpSocket c;
    if (!tryConnect(c)) abort();
    c.write(reinterpret_cast<const char *>(data), static_cast<qint64>(size));
    // The server answers every request it reads and then closes the connection.
    while (c.state() != QAbstractSocket::UnconnectedState) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
        c.readAll();
    }
    delete server;
    server = nullptr;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return 0;
}
