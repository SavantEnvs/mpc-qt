// Playlist files: the playlists.json collection (PlaylistCollection::fromVList ->
// Playlist::fromVMap -> Item::fromVMap, as main.cpp restores it) and .m3u playlists
// (Storage::readM3U -> Playlist::fromStringList), each followed by the save path
// (toVList / toStringList) and shuffle/unshuffle.
// Input byte 0: even = JSON playlists document, odd = M3U file contents; the rest is the file.
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryFile>
#include <QDir>
#include "playlist.h"
#include "storage.h"
#include "fuzz_common.h"

extern "C" int LLVMFuzzerInitialize(int *, char ***) { mpcqt_fuzz_init(); return 0; }

static void dropItems(const QSharedPointer<Playlist> &pl) {
    // ItemCollection is a process-wide registry; unregister what this input added.
    QList<QUuid> uuids;
    pl->iterateItems([&uuids](QSharedPointer<Item> i) { uuids.append(i->uuid()); });
    for (const auto &u : uuids) ItemCollection::getSingleton()->removeItem(u);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    mpcqt_fuzz_init();
    if (size < 1) return -1;
    QByteArray body(reinterpret_cast<const char *>(data) + 1, static_cast<qsizetype>(size - 1));

    if ((data[0] & 1) == 0) {
        QVariantList vl = QJsonDocument::fromJson(body).array().toVariantList();
        auto coll = PlaylistCollection::getBackup();
        coll->removeAllPlaylists();
        coll->fromVList(vl);
        (void)coll->toVList();
        coll->iteratePlaylists([](QSharedPointer<Playlist> pl) {
            (void)pl->toStringList();
            pl->shuffleItems();
            pl->unshuffleItems();
            (void)pl->toVMap();
            dropItems(pl);
        });
        coll->removeAllPlaylists();
    } else {
        QTemporaryFile f(QDir::tempPath() + "/mpcqt-fuzz-XXXXXX.m3u");
        if (!f.open()) return 0;
        f.write(body);
        f.flush();
        Storage storage;
        QStringList lines = storage.readM3U(f.fileName());
        QSharedPointer<Playlist> pl(new Playlist(QStringLiteral("m3u")));
        pl->fromStringList(lines);
        (void)pl->toStringList();
        (void)pl->toVMap();
        dropItems(pl);
    }
    return 0;
}
