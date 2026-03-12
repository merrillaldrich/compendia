#include "multiprogressbar.h"

#include <QHBoxLayout>

/*! \brief Constructs the widget, creating the internal label, bar, and cycle timer.
 *
 * \param parent Optional Qt parent widget.
 */
MultiProgressBar::MultiProgressBar(QWidget *parent)
    : QWidget(parent)
    , label_(new QLabel("Progress:", this))
    , bar_(new QProgressBar(this))
    , cycle_timer_(new QTimer(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addStretch(1);
    layout->addWidget(label_);
    layout->addWidget(bar_);

    bar_->setFixedSize(300, 12);
    bar_->setTextVisible(false);
    bar_->setMinimum(0);
    bar_->setMaximum(1);
    bar_->setValue(0);

    cycle_timer_->setInterval(cycle_interval_ms_);
    connect(cycle_timer_, &QTimer::timeout, this, &MultiProgressBar::onCycleTimer);
    cycle_timer_->start();
}

/*! \brief Registers (or re-registers) a process and immediately shows it.
 *
 * \param p     The process to start.
 * \param min   Minimum progress value.
 * \param max   Maximum progress value.
 * \param label Status label text for this process.
 */
void MultiProgressBar::startProcess(Process p, int min, int max, const QString &label)
{
    states_[p] = { min, max, min, label, true };

    if (!active_.contains(p))
        active_.append(p);

    cycle_timer_->setInterval(cycle_interval_ms_);
    updateDisplay();
}

/*! \brief Increments the process value by 1 and auto-finishes when it reaches max.
 *
 * When the value reaches max, the display is first updated to show the full/green
 * state, then finishProcess() is scheduled after a short delay so the user can
 * see the completion before the bar resets.
 *
 * \param p The process to advance.
 */
void MultiProgressBar::increment(Process p)
{
    if (!states_[p].active)
        return;

    states_[p].value++;
    updateDisplay();

    if (states_[p].value >= states_[p].max) {
        // Briefly show the green (full) state before clearing.
        QTimer::singleShot(600, this, [this, p]() {
            if (states_[p].active && states_[p].value >= states_[p].max)
                finishProcess(p);
        });
    }
}

/*! \brief Explicitly marks a process as done and emits processFinished().
 *
 * \param p The process to finish.
 */
void MultiProgressBar::finishProcess(Process p)
{
    states_[p].active = false;
    active_.removeAll(p);
    cycle_index_ = qMin(cycle_index_, qMax(0, active_.size() - 1));

    emit processFinished(p);
    updateDisplay();
}

void MultiProgressBar::setLabel(Process p, const QString &label)
{
    if (!states_[p].active)
        return;
    states_[p].label = label;
    updateDisplay();
}

int MultiProgressBar::value(Process p) const
{
    return states_.contains(p) ? states_[p].value : 0;
}

int MultiProgressBar::max(Process p) const
{
    return states_.contains(p) ? states_[p].max : 0;
}

/*! \brief Changes the label-cycling interval.
 *
 * \param ms Interval in milliseconds.
 */
void MultiProgressBar::setCycleInterval(int ms)
{
    cycle_interval_ms_ = ms;
    cycle_timer_->setInterval(ms);
}

/*! \brief Advances the cycle index and refreshes the display. */
void MultiProgressBar::onCycleTimer()
{
    if (active_.size() > 1)
        cycle_index_ = (cycle_index_ + 1) % active_.size();

    updateDisplay();
}

/*! \brief Pushes the currently cycled process state to label_ and bar_. */
void MultiProgressBar::updateDisplay()
{
    if (active_.isEmpty()) {
        label_->setText("Progress:");
        bar_->setMaximum(1);
        bar_->setValue(0);
        return;
    }

    Process p = active_[cycle_index_ % active_.size()];
    const ProcessState &s = states_[p];
    label_->setText(s.label);
    bar_->setMinimum(s.min);
    bar_->setMaximum(s.max);
    bar_->setValue(s.value);
}
