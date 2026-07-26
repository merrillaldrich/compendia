#include "locationtagger.h"
#include "compendiacore.h"
#include "geo.h"
#include "taggedfile.h"

LocationTagger::LocationTagger(CompendiaCore *core, QObject *parent)
    : QObject(parent)
    , core_(core)
{}

void LocationTagger::start(const QList<Entry> &queue)
{
    queue_   = queue;
    total_   = queue_.size();
    done_    = 0;
    tagged_  = 0;
    aborted_ = false;

    if (!timer_) {
        timer_ = new QTimer(this);
        timer_->setSingleShot(false);
        connect(timer_, &QTimer::timeout, this, [this]() {
            if (queue_.isEmpty()) {
                timer_->stop();
                return;
            }

            const Entry entry = queue_.takeFirst();

            Geo::reverseGeocode(entry.lat, entry.lon, this,
                [this, tf = entry.tf](QString city, QString state, QString country, QString error) {
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
                        if (!aborted_)
                            emit progress(done_, total_);
                    }

                    if (done_ == total_)
                        emit finished(tagged_, aborted_);
                });
        });
    }

    timer_->start(0);
}
