#include "framegrabber.h"

#include <QDateTime>
#include <QDebug>
#include <QMediaMetaData>
#include <QTimer>
#include <QUrl>
#include <QVideoFrame>

/*! \brief Converts a QMediaMetaData object to a QMap<QString,QString>.
 *
 * QDateTime values are formatted as ISO 8601; QSize values as "WxH"; all other
 * types are converted via QVariant::toString().
 *
 * \param meta The metadata object returned by QMediaPlayer::metaData().
 * \return A flat string map with human-readable key names.
 */
static QMap<QString, QString> mediaMetaToMap(const QMediaMetaData &meta)
{
    QMap<QString, QString> map;
    for (QMediaMetaData::Key key : meta.keys()) {
        QString keyStr = QMediaMetaData::metaDataKeyToString(key);
        QVariant val   = meta.value(key);

        if (val.typeId() == QMetaType::QDateTime)
            map.insert(keyStr, val.toDateTime().toString(Qt::ISODate));
        else if (val.typeId() == QMetaType::QSize)
            map.insert(keyStr, QString("%1x%2").arg(val.toSize().width())
                                               .arg(val.toSize().height()));
        else
            map.insert(keyStr, val.toString());
    }
    return map;
}

/*! \brief Constructs a FrameGrabber.
 *
 * \param parent Qt parent object.
 */
FrameGrabber::FrameGrabber(QObject *parent)
    : QObject(parent)
{}

/*! \brief Destroys the FrameGrabber. */
FrameGrabber::~FrameGrabber() = default;

/*! \brief Sets the target seek position used when the video is longer than \p ms milliseconds.
 *
 * \param ms Seek position in milliseconds.
 */
void FrameGrabber::setSeekMs(qint64 ms)
{
    seekMs_ = ms;
}

/*! \brief Begins asynchronous frame capture for the given list of video paths.
 *
 * Creates a single QMediaPlayer on the main thread that is reused for the entire
 * batch, then schedules processNext() to start the first file.
 *
 * \param absolutePaths Absolute paths to the video files to process.
 */
void FrameGrabber::grab(const QStringList &absolutePaths)
{
    paths_        = absolutePaths;
    currentIndex_ = 0;
    successCount_ = 0;
    failCount_    = 0;
    processing_   = false;

    player_  = new QMediaPlayer(this);
    sink_    = new QVideoSink(this);
    timeout_ = new QTimer(this);

    player_->setVideoSink(sink_);
    timeout_->setSingleShot(true);
    timeout_->setInterval(15000);

    connect(sink_,    &QVideoSink::videoFrameChanged,    this, &FrameGrabber::onVideoFrameChanged);
    connect(player_,  &QMediaPlayer::mediaStatusChanged, this, &FrameGrabber::onMediaStatusChanged);
    connect(player_,  &QMediaPlayer::errorOccurred,      this, &FrameGrabber::onPlayerError);
    connect(timeout_, &QTimer::timeout,                  this, &FrameGrabber::onTimeout);

    QTimer::singleShot(0, this, &FrameGrabber::processNext);
}

/*! \brief Loads the next path into the player, or emits finished() if the batch is done. */
void FrameGrabber::processNext()
{
    if (currentIndex_ >= paths_.size()) {
        emit finished(successCount_, failCount_);
        return;
    }

    processing_   = true;
    currentFrame_ = QImage();
    currentMeta_.clear();

    const QString &path = paths_[currentIndex_];
    qDebug() << "[FrameGrabber] processNext:" << path;

    player_->setSource(QUrl::fromLocalFile(path));
    timeout_->start();
}

/*! \brief Reacts to player media-status changes (seeks and plays when loaded, fails on error). */
void FrameGrabber::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (!processing_) return;

    if (status == QMediaPlayer::LoadedMedia) {
        currentMeta_ = mediaMetaToMap(player_->metaData());

        qint64 dur = player_->duration();
        qint64 seekPos;
        if (dur > 0 && dur <= seekMs_)
            seekPos = dur / 2;   // short video — go to midpoint
        else
            seekPos = seekMs_;   // normal video — use configured offset

        qDebug() << "[FrameGrabber]  duration=" << dur
                 << "ms, seekPos=" << seekPos << "ms";

        player_->setPosition(seekPos);
        player_->play();

    } else if (status == QMediaPlayer::EndOfMedia
               || status == QMediaPlayer::InvalidMedia) {
        if (!currentFrame_.isNull()) return; // frame already captured, ignore
        finishCurrentFile(false, QStringLiteral("Media ended without a valid frame"));
    }
}

/*! \brief Captures the first valid video frame and finishes the current file. */
void FrameGrabber::onVideoFrameChanged(const QVideoFrame &frame)
{
    if (!processing_ || !currentFrame_.isNull()) return;
    if (!frame.isValid()) return;

    QVideoFrame f = frame;
    currentFrame_ = f.toImage();

    if (currentFrame_.isNull()) {
        qDebug() << "[FrameGrabber]  frame conversion failed for" << paths_[currentIndex_];
        return; // keep waiting; timeout will fire eventually
    }

    currentFrame_ = currentFrame_.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    qDebug() << "[FrameGrabber]  frame captured, size" << currentFrame_.size();

    finishCurrentFile(true);
}

/*! \brief Finishes the current file with failure when the player reports an error. */
void FrameGrabber::onPlayerError(QMediaPlayer::Error, const QString &errorString)
{
    if (!processing_) return;
    qDebug() << "[FrameGrabber]  error:" << errorString;
    finishCurrentFile(false, errorString);
}

/*! \brief Finishes the current file with failure when the per-file timeout fires. */
void FrameGrabber::onTimeout()
{
    if (!processing_) return;
    qDebug() << "[FrameGrabber]  timeout for" << paths_[currentIndex_];
    finishCurrentFile(false, QStringLiteral("Timeout"));
}

/*! \brief Closes out the current file, emits signals, and schedules processNext().
 *
 * \param success True if a valid frame was captured.
 * \param reason  Failure description used when \p success is false.
 */
void FrameGrabber::finishCurrentFile(bool success, const QString &reason)
{
    if (!processing_) return;
    processing_ = false; // set first so late-arriving signals are ignored

    timeout_->stop();
    player_->stop();
    // Clear the source so Qt's FFmpeg backend resets its interrupt callback.
    // Without this, the "abort" flag armed by stop() is still set when the
    // next setSource() call begins, causing an immediate AVERROR_EXIT failure.
    player_->setSource(QUrl());

    const QString &path = paths_[currentIndex_];

    if (success) {
        ++successCount_;
        emit frameGrabbed(path, currentFrame_, currentMeta_);
    } else {
        ++failCount_;
        emit frameFailed(path, reason.isEmpty() ? QStringLiteral("No frame captured") : reason,
                         currentMeta_);
    }

    emit progress(currentIndex_ + 1, paths_.size());
    ++currentIndex_;

    // Yield to the event loop before processing the next file so that UI
    // updates (icon refresh, progress bar) are painted between files.
    QTimer::singleShot(0, this, &FrameGrabber::processNext);
}
