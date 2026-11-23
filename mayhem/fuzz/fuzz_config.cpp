// Settings / keys / recent-files JSON deserializers in src/helpers.cpp:
//   MouseState::fromVMap (keys.json mouse bindings) -> toString / toVMap / mouseHash
//   TrackInfo::tracksFromVList (recent.json) -> tracksToVList
//   AudioDevice::listFromVList (mpv audio-device-list) -> displayString
//   Helpers::vmapToRect / vmapToSize (geometry.json)
// Input: a JSON document; objects and arrays inside it are handed to each deserializer.
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRect>
#include "helpers.h"
#include "fuzz_common.h"

extern "C" int LLVMFuzzerInitialize(int *, char ***) { mpcqt_fuzz_init(); return 0; }

static void eatMap(const QVariantMap &m) {
    MouseState ms;
    ms.fromVMap(m);
    (void)ms.toString();
    (void)ms.toVMap();
    (void)ms.mouseHash();
    (void)ms.isPress(); (void)ms.isTwice(); (void)ms.isWheel();
    (void)ms.mouseButtons(); (void)ms.keyModifiers();
    TrackInfo ti;
    ti.fromVMap(m);
    (void)ti.toVMap();
    AudioDevice ad(m);
    (void)ad.displayString();
    (void)ad.deviceName();
    QRect r = Helpers::vmapToRect(m);
    (void)Helpers::rectToVmap(r);
    QSize s = Helpers::vmapToSize(m);
    (void)Helpers::sizeToVmap(s);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    mpcqt_fuzz_init();
    QJsonDocument doc = QJsonDocument::fromJson(
        QByteArray(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size)));
    if (doc.isObject()) {
        QVariantMap top = doc.object().toVariantMap();
        eatMap(top);
        for (auto it = top.cbegin(); it != top.cend(); ++it) {
            if (it.value().typeId() == QMetaType::QVariantMap) eatMap(it.value().toMap());
            if (it.value().typeId() == QMetaType::QVariantList) {
                QVariantList l = it.value().toList();
                (void)TrackInfo::tracksToVList(TrackInfo::tracksFromVList(l));
                for (const auto &ad : AudioDevice::listFromVList(l)) (void)ad.displayString();
            }
        }
    } else if (doc.isArray()) {
        QVariantList l = doc.array().toVariantList();
        (void)TrackInfo::tracksToVList(TrackInfo::tracksFromVList(l));
        for (const auto &ad : AudioDevice::listFromVList(l)) (void)ad.displayString();
        for (const auto &v : l) eatMap(v.toMap());
    }
    return 0;
}
