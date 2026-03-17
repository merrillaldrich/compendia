#ifndef MULTIPROGRESSBAR_H
#define MULTIPROGRESSBAR_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QMap>
#include <QList>

/*! \brief A status-bar widget that tracks multiple concurrent background processes
 *         and cycles the display between them when more than one is active.
 *
 * Each process is identified by the \c Process enum.  Callers register a process
 * with startProcess(), advance it with increment(), and optionally terminate it
 * early with finishProcess().  When two processes are active simultaneously the
 * label and bar alternate on a configurable timer.
 */
class MultiProgressBar : public QWidget
{
    Q_OBJECT

public:
    /*! \brief Identifies a background process tracked by this widget. */
    enum class Process { FolderScan, IconGeneration, Save, EmbeddingWarmup, VideoGrab, FaceDetection };
    Q_ENUM(Process)

    /*! \brief Constructs the widget, creating the internal label, bar, and cycle timer.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit MultiProgressBar(QWidget *parent = nullptr);

    /*! \brief Registers (or re-registers) a process and immediately shows it.
     *
     * Call this before background work begins so the bar has a meaningful range.
     * Re-registering a still-active process resets its counters in-place.
     *
     * \param p     The process to start.
     * \param min   Minimum progress value.
     * \param max   Maximum progress value (completion threshold).
     * \param label Text shown in the status-bar label while this process is active.
     */
    void startProcess(Process p, int min, int max, const QString &label);

    /*! \brief Increments the process value by 1 and auto-finishes when it reaches max.
     *
     * \param p The process to advance.
     */
    void increment(Process p);

    /*! \brief Explicitly marks a process as done.
     *
     * Also called internally by increment() when the value reaches max.
     *
     * \param p The process to finish.
     */
    void finishProcess(Process p);

    /*! \brief Updates the label text for an active process without resetting its counters.
     *
     * Does nothing if the process is not currently active.
     * \param p     The process whose label to update.
     * \param label The new label text.
     */
    void setLabel(Process p, const QString &label);

    /*! \brief Returns the current progress value for \p p, or 0 if not active. */
    int value(Process p) const;

    /*! \brief Returns the maximum progress value for \p p, or 0 if not active. */
    int max(Process p) const;

    /*! \brief Changes the label-cycling interval.
     *
     * \param ms Interval in milliseconds; default is 2000.
     */
    void setCycleInterval(int ms);

signals:
    /*! \brief Emitted when a process finishes (either via increment reaching max or finishProcess). */
    void processFinished(Process p);

private slots:
    /*! \brief Advances the cycle index and refreshes the display. */
    void onCycleTimer();

private:
    /*! \brief Holds runtime state for a single tracked process. */
    struct ProcessState {
        int     min    = 0;     ///< Minimum progress value (usually 0).
        int     max    = 1;     ///< Maximum progress value (completion threshold).
        int     value  = 0;     ///< Current progress value.
        QString label;          ///< Text displayed in the status label while this process is active.
        bool    active = false; ///< True while this process is registered and not yet finished.
    };

    /*! \brief Pushes the currently cycled process state to label_ and bar_. */
    void updateDisplay();

    QLabel        *label_;          ///< Status label showing the name of the currently displayed process.
    QProgressBar  *bar_;            ///< Progress bar reflecting the current process value.
    QTimer        *cycle_timer_;    ///< Timer that advances cycle_index_ when multiple processes are active.
    QMap<Process, ProcessState> states_; ///< State for each known process, keyed by Process enum value.
    QList<Process>              active_;   ///< Active processes in start order.
    int cycle_index_       = 0;    ///< Index into active_ indicating which process is currently displayed.
    int cycle_interval_ms_ = 2000; ///< Milliseconds between display switches when cycling.
};

#endif // MULTIPROGRESSBAR_H
