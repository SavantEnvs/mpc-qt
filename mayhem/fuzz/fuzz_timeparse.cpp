// Time/date and WxH / X,Y string parsers in src/helpers.cpp:
//   Helpers::fromDateFormat  (Go To window "hh:mm:ss" text box)
//   Helpers::dateFromCFormat / timeFromCFormat ("Mmm dd yyyy" / "hh:mm:ss")
//   Helpers::sizeFromString / pointFromString (--size / --pos CLI values)
// plus the toDateFormat* round trip of a parsed time.
#include <string>
#include <QSize>
#include <QPoint>
#include "helpers.h"
#include "fuzz_common.h"

extern "C" int LLVMFuzzerInitialize(int *, char ***) { mpcqt_fuzz_init(); return 0; }

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    mpcqt_fuzz_init();
    std::string s(reinterpret_cast<const char *>(data), size);  // NUL-terminated copy
    QString q = QString::fromUtf8(s.data(), static_cast<qsizetype>(s.size()));

    double t = Helpers::fromDateFormat(q);
    if (t >= 0) {
        (void)Helpers::toDateFormat(t);
        (void)Helpers::toDateFormatWithZero(t);
        (void)Helpers::toDateFormatFixed(t, Helpers::LongFormat);
        (void)Helpers::toDateFormatFixed(t, Helpers::ShortHourFormat);
    }
    QDate d = Helpers::dateFromCFormat(s.c_str());
    (void)d.toString();
    QTime tm = Helpers::timeFromCFormat(s.c_str());
    (void)tm.toString();

    QSize sz;
    (void)Helpers::sizeFromString(sz, q);
    QPoint pt;
    (void)Helpers::pointFromString(pt, q);
    return 0;
}
