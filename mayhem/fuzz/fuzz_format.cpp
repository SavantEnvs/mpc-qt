// The two template mini-languages in src/helpers.cpp:
//   DisplayParser::takeFormatString + parseMetadata — the playlist title format
//     (%prop{..}{..}{..} with #/$ escapes) evaluated against a media-tag metadata map
//   Helpers::parseFormat — the screenshot / thumbnail file-name template (%f, %t.., ...)
// Input: <format string> NUL <metadata JSON object>. The trailing 10 bytes (if present) pick the
// playback state passed to parseFormat (times are bounded like real playback positions).
#include <cstring>
#include <QJsonDocument>
#include <QJsonObject>
#include <fuzzer/FuzzedDataProvider.h>
#include "helpers.h"
#include "fuzz_common.h"

extern "C" int LLVMFuzzerInitialize(int *, char ***) { mpcqt_fuzz_init(); return 0; }

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    mpcqt_fuzz_init();
    const char *p = reinterpret_cast<const char *>(data);
    const void *nul = memchr(data, 0, size);
    size_t fmtLen = nul ? static_cast<const uint8_t *>(nul) - data : size;
    QString fmt = QString::fromUtf8(p, static_cast<qsizetype>(fmtLen));
    QByteArray rest = nul ? QByteArray(p + fmtLen + 1, static_cast<qsizetype>(size - fmtLen - 1))
                          : QByteArray();

    QVariantMap meta = QJsonDocument::fromJson(rest).object().toVariantMap();
    DisplayParser dp;
    dp.takeFormatString(fmt);
    (void)dp.parseMetadata(meta, QStringLiteral("Some File Name.mkv"), Helpers::VideoFile);
    (void)dp.parseMetadata(meta, QStringLiteral("song.flac"), Helpers::AudioFile);

    FuzzedDataProvider fdp(reinterpret_cast<const uint8_t *>(rest.constData()),
                           static_cast<size_t>(rest.size()));
    auto disabled = static_cast<Helpers::DisabledTrack>(fdp.ConsumeIntegralInRange<int>(0, 2));
    auto subs = static_cast<Helpers::Subtitles>(fdp.ConsumeIntegralInRange<int>(0, 2));
    // parseFormat is called with playback positions in seconds (manager.cpp); keep them in range.
    double tNav = fdp.ConsumeFloatingPointInRange<double>(0, 1e6);
    double tBeg = fdp.ConsumeFloatingPointInRange<double>(0, 1e6);
    double tEnd = fdp.ConsumeFloatingPointInRange<double>(0, 1e6);
    (void)Helpers::parseFormat(fmt, QStringLiteral("/media/movies/Some File Name.mkv"),
                               disabled, subs, tNav, tBeg, tEnd);
    return 0;
}
