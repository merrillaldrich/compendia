#ifndef FRAMEGRABBER_H
#define FRAMEGRABBER_H

#include <QObject>
#include <QImage>
#include <QStringList>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QTimer>

/*! \brief Captures a representative frame from each video file in a batch.
 *
 * Operates entirely on the main thread using an async state machine so that
 * the QMediaPlayer it creates shares the same COM/WMF apartment as every other
 * QMediaPlayer in the application, avoiding apartment-model conflicts on Windows.
 *
 * Call grab() to begin a batch.  Results, progress ticks, and the final summary
 * arrive as signal emissions on the main thread.  The class is single-use per
 * batch: create a new instance for each grab operation and call deleteLater()
 * when finished() fires.
 */
class FrameGrabber : public QObject
{
    Q_OBJECT

public:
    /*! \brief Constructs a FrameGrabber.
     *
     * \param parent Qt parent object.
     */
    explicit FrameGrabber(QObject *parent = nullptr);

    /*! \brief Destroys the FrameGrabber. */
    ~FrameGrabber();

    /*! \brief Sets the target seek position used when the video is longer than \p ms milliseconds.
     *
     * For videos shorter than or equal to \p ms the midpoint is used instead.
     * Default is 2000 ms.
     *
     * \param ms Seek position in milliseconds.
     */
    void setSeekMs(qint64 ms);

    /*! \brief Begins asynchronous frame capture for the given list of video paths.
     *
     * Returns immediately; results arrive via frameGrabbed(), frameFailed(),
     * progress(), and finished() signals on the main thread.
     *
     * \param absolutePaths Absolute paths to the video files to process.
     */
    void grab(const QStringList &absolutePaths);

signals:
    /*! \brief Emitted after a frame is successfully captured from \p absolutePath.
     *
     * \param absolutePath The video file that was processed.
     * \param frame        The captured frame scaled to at most 400×400 px.
     */
    void frameGrabbed(const QString &absolutePath, const QImage &frame);

    /*! \brief Emitted when frame capture fails for \p absolutePath.
     *
     * \param absolutePath The video file that could not be processed.
     * \param reason       A human-readable description of the failure.
     */
    void frameFailed(const QString &absolutePath, const QString &reason);

    /*! \brief Emitted after each file is processed (success or failure).
     *
     * \param done  Number of files processed so far.
     * \param total Total number of files in the batch.
     */
    void progress(int done, int total);

    /*! \brief Emitted once all files in the batch have been processed.
     *
     * \param successCount Number of files from which a frame was captured.
     * \param failCount    Number of files for which capture failed.
     */
    void finished(int successCount, int failCount);

private slots:
    /*! \brief Loads the next path into the player, or emits finished() if the batch is done. */
    void processNext();

    /*! \brief Reacts to player media-status changes (seeks and plays when loaded, fails on error). */
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    /*! \brief Captures the first valid video frame and finishes the current file. */
    void onVideoFrameChanged(const QVideoFrame &frame);

    /*! \brief Finishes the current file with failure when the player reports an error. */
    void onPlayerError(QMediaPlayer::Error error, const QString &errorString);

    /*! \brief Finishes the current file with failure when the per-file timeout fires. */
    void onTimeout();

private:
    /*! \brief Closes out the current file, emits signals, and schedules processNext().
     *
     * Sets \c processing_ to false before emitting anything so that any queued
     * signals arriving late are silently ignored.
     *
     * \param success True if a valid frame was captured.
     * \param reason  Failure description used when \p success is false.
     */
    void finishCurrentFile(bool success, const QString &reason = {});

    qint64        seekMs_       = 2000;   ///< Target seek position in milliseconds.
    QMediaPlayer *player_       = nullptr;
    QVideoSink   *sink_         = nullptr;
    QTimer       *timeout_      = nullptr;
    QStringList   paths_;
    int           currentIndex_ = 0;
    int           successCount_ = 0;
    int           failCount_    = 0;
    bool          processing_   = false;  ///< True while a file is being processed.
    QImage        currentFrame_;          ///< Frame captured for the current file.
};

#endif // FRAMEGRABBER_H
