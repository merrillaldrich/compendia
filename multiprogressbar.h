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
    enum class Process { IconGeneration, Save };
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
        int     min    = 0;
        int     max    = 1;
        int     value  = 0;
        QString label;
        bool    active = false;
    };

    /*! \brief Pushes the currently cycled process state to label_ and bar_. */
    void updateDisplay();

    QLabel        *label_;
    QProgressBar  *bar_;
    QTimer        *cycle_timer_;
    QMap<Process, ProcessState> states_;
    QList<Process>              active_;   ///< Active processes in start order.
    int cycle_index_       = 0;
    int cycle_interval_ms_ = 2000;
};

#endif // MULTIPROGRESSBAR_H
