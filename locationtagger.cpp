#include "locationtagger.h"
#include "compendiacore.h"
#include "geo.h"
#include "taggedfile.h"

#include <QtGlobal>

static constexpr int kMaxConcurrent = 8;

LocationTagger::LocationTagger(CompendiaCore *core, QObject *parent)
    : QObject(parent)
    , core_(core)
{}

void LocationTagger::start(const QList<Entry> &queue)
{
    queue_    = queue;
    total_    = queue_.size();
    done_     = 0;
    tagged_   = 0;
    aborted_  = false;
    inFlight_ = 0;

    const int initial = qMin(kMaxConcurrent, total_);
    for (int i = 0; i < initial; ++i)
        dispatchNext();
}

void LocationTagger::dispatchNext()
{
    if (queue_.isEmpty() || aborted_)
        return;

    ++inFlight_;
    const Entry entry = queue_.takeFirst();

    Geo::reverseGeocode(entry.lat, entry.lon, this,
        [this, tf = entry.tf](QString city, QString state, QString country, QString error) {
            --inFlight_;
            ++done_;

            if (!error.isEmpty()) {
                if (!aborted_) {
                    aborted_ = true;
                    emit errorOccurred(error);
                }
            } else {
                bool tagged = false;
                if (!city.isEmpty()) {
                    tf->addTag(core_->addLibraryTag(QStringLiteral("City"), city));
                    tagged = true;
                }
                if (!state.isEmpty()) {
                    tf->addTag(core_->addLibraryTag(QStringLiteral("State/Province"), state));
                    tagged = true;
                }
                if (!country.isEmpty()) {
                    tf->addTag(core_->addLibraryTag(QStringLiteral("Country"), country));
                    tagged = true;
                }
                if (tagged) ++tagged_;
                emit progress(done_, total_);
            }

            if (done_ == total_ || (aborted_ && inFlight_ == 0)) {
                emit finished(tagged_, aborted_);
            } else if (!aborted_) {
                dispatchNext();  // keep the window full
            }
        });
}
