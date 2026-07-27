#ifndef LOCATIONTAGGER_H
#define LOCATIONTAGGER_H

#include <QList>
#include <QObject>

class CompendiaCore;
class TaggedFile;

/*! \brief Asynchronous worker that reverse-geocodes a batch of GPS-tagged files and
 *         applies City, State/Province, and Country tags via CompendiaCore.
 *
 * Callers build a list of Entry values (each holding a TaggedFile pointer and its
 * coordinates), then call start().  Progress, errors, and completion are reported
 * through signals so the caller can update the UI without knowing the internals.
 *
 * The worker uses a sliding window of concurrent async requests (no dedicated
 * thread or timer) and is safe to parent to the MainWindow; destroy it via
 * deleteLater() after finished() is received.
 */
class LocationTagger : public QObject
{
    Q_OBJECT

public:
    /*! \brief One file queued for reverse geocoding. */
    struct Entry {
        TaggedFile *tf;
        double lat;
        double lon;
    };

    /*! \brief Constructs a LocationTagger.
     *
     * \param core   CompendiaCore used to create library tags.
     * \param parent Optional Qt parent.
     */
    explicit LocationTagger(CompendiaCore *core, QObject *parent = nullptr);

    /*! \brief Starts draining \a queue using a sliding window of concurrent requests.
     *
     * \param queue Files to geocode; must be non-empty.
     */
    void start(const QList<Entry> &queue);

signals:
    /*! \brief Emitted after each successful geocode response.
     *
     * \param done  Number of responses received so far.
     * \param total Total entries in the batch.
     */
    void progress(int done, int total);

    /*! \brief Emitted once when the first network error is received.
     *
     * \param message Human-readable error description from the map service.
     */
    void errorOccurred(QString message);

    /*! \brief Emitted when all responses have arrived.
     *
     * \param tagged   Number of files that received at least one location tag.
     * \param hadError True if at least one error was received during the batch.
     */
    void finished(int tagged, bool hadError);

private:
    void dispatchNext();

    CompendiaCore *core_;
    QList<Entry>   queue_;
    int            inFlight_ = 0;
    int            done_     = 0;
    int            tagged_   = 0;
    int            total_    = 0;
    bool           aborted_  = false;
};

#endif // LOCATIONTAGGER_H
