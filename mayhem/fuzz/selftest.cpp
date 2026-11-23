// Known-answer selftest for the parsers the fuzz targets exercise (mpc-qt ships no unit tests).
// Prints one "<case>\t<result>" line per case; mayhem/test.sh diffs them against
// mayhem/selftest.expected, so a neutered binary (no/empty output) fails every case.
#include <cstdio>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryFile>
#include <QDir>
#include "helpers.h"
#include "playlist.h"
#include "storage.h"

static void out(const char *name, const QString &v) {
    printf("%s\t%s\n", name, v.toUtf8().constData());
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    out("fromDateFormat.hms", QString::number(Helpers::fromDateFormat("01:02:03")));
    out("fromDateFormat.short", QString::number(Helpers::fromDateFormat("1:2")));
    out("fromDateFormat.big", QString::number(Helpers::fromDateFormat("00:61:00")));
    out("timeFromCFormat", Helpers::timeFromCFormat("12:34:56").toString("hh:mm:ss"));
    out("timeFromCFormat.short", Helpers::timeFromCFormat("12:34").isValid() ? "valid" : "invalid");
    out("dateFromCFormat", Helpers::dateFromCFormat("Mar 15 2021").toString(Qt::ISODate));
    out("dateFromCFormat.short", Helpers::dateFromCFormat("Mar").isValid() ? "valid" : "invalid");
    out("toDateFormat", Helpers::toDateFormat(3723.5));
    out("toDateFormatFixed", Helpers::toDateFormatFixed(65, Helpers::LongHourFormat));
    QSize sz; bool okS = Helpers::sizeFromString(sz, "640x480");
    out("sizeFromString", QString("%1 %2 %3").arg(okS).arg(sz.width()).arg(sz.height()));
    QPoint pt; bool okP = Helpers::pointFromString(pt, "10,20");
    out("pointFromString", QString("%1 %2 %3").arg(okP).arg(pt.x()).arg(pt.y()));
    out("parseFormat", Helpers::parseFormat("%F-%tp{}%s{subs}{nosubs}", "/m/Movie.mkv",
        Helpers::NothingDisabled, Helpers::SubtitlesPresent, 3723.25, 0, 10));
    DisplayParser dp;
    dp.takeFormatString("%artist{# - }{Unknown Artist - }{}%title{#}{$}{$}");
    QVariantMap meta{{"artist", "Band"}, {"title", "Song"}};
    out("displayParser", dp.parseMetadata(meta, "file.flac", Helpers::AudioFile));
    QVariantMap meta2{{"album", "X"}};
    out("displayParser.noartist", dp.parseMetadata(meta2, "file.flac", Helpers::AudioFile));

    QSharedPointer<Playlist> pl(new Playlist("t"));
    pl->fromStringList({"/a/b.mkv", "http://example.com/c.mp3"});
    out("m3u.roundtrip", pl->toStringList().join("|"));
    QTemporaryFile f(QDir::tempPath() + "/selftest-XXXXXX.m3u");
    f.open(); f.write("#EXTM3U\n\n  /x/one.mkv  \n#comment\n/x/two.mkv\n"); f.flush();
    Storage storage;
    out("readM3U", storage.readM3U(f.fileName()).join("|"));

    QByteArray js = R"([{"title":"Mine","repeat":true,"items":[{"url":"file:///v/a.mkv"},{"url":"file:///v/b.mkv"}]}])";
    auto coll = PlaylistCollection::getBackup();
    coll->removeAllPlaylists();
    coll->fromVList(QJsonDocument::fromJson(js).array().toVariantList());
    auto p0 = coll->playlistAt(0);
    out("playlist.json", p0 ? QString("%1 %2 %3 %4").arg(coll->count()).arg(p0->title())
        .arg(p0->repeat()).arg(p0->toStringList().join("|")) : QString("none"));

    MouseState ms; ms.fromVMap({{"button", 2}, {"mod", 1}, {"press", 1}});
    out("mouseState", ms.toString());
    return 0;
}
