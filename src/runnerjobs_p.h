/*
 *   Copyright (C) 2007, 2009 Ryan P. Bitanga <ryan.bitanga@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLASMA_RUNNERJOBS_P_H
#define PLASMA_RUNNERJOBS_P_H

#include <QHash>
#include <QMutex>
#include <QSet>

#include <ThreadWeaver/Job>
#include <ThreadWeaver/QueuePolicy>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/QObjectDecorator>

#include "abstractrunner.h"

using ThreadWeaver::Job;

class QTimer;

namespace Plasma {
// Queue policies

// QueuePolicy that only allows a job to be executed after
// waiting in the queue for the specified timeout
class DelayedRunnerPolicy : public ThreadWeaver::QueuePolicy
{
public:
    ~DelayedRunnerPolicy();

    static DelayedRunnerPolicy &instance();

    bool canRun(ThreadWeaver::JobPointer job) override;
    void free(ThreadWeaver::JobPointer job) override;
    void release(ThreadWeaver::JobPointer job) override;
    void destructed(ThreadWeaver::JobInterface* job) override;

private:
    DelayedRunnerPolicy();
    QMutex m_mutex;
};

// QueuePolicy that limits the instances of a particular runner
class DefaultRunnerPolicy : public ThreadWeaver::QueuePolicy
{
public:
    ~DefaultRunnerPolicy() override;

    static DefaultRunnerPolicy &instance();

    void setCap(int cap)
    {
        m_cap = cap;
    }
    int cap() const
    {
        return m_cap;
    }

    bool canRun(ThreadWeaver::JobPointer job) override;
    void free(ThreadWeaver::JobPointer job) override;
    void release(ThreadWeaver::JobPointer job) override;
    void destructed(ThreadWeaver::JobInterface* job) override;
private:
    DefaultRunnerPolicy();

    int m_cap;
    QHash<QString, int> m_runCounts;
    QMutex m_mutex;
};

/*
 * FindMatchesJob class
 * Class to run queries in different threads
 */
class FindMatchesJob : public Job
{
public:
    FindMatchesJob(Plasma::AbstractRunner *runner,
                   Plasma::RunnerContext *context, QObject *parent = nullptr);
    ~FindMatchesJob() override;

    int priority() const override;
    Plasma::AbstractRunner* runner() const;

    QTimer* delayTimer() const;
    void setDelayTimer(QTimer *timer);
    ThreadWeaver::QObjectDecorator* decorator() const { return m_decorator; }

protected:
    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread* thread) override;

private:
    Plasma::RunnerContext m_context;
    Plasma::AbstractRunner *m_runner;
    QTimer *m_timer;
    ThreadWeaver::QObjectDecorator* m_decorator;
};

class DelayedJobCleaner : public QObject
{
    Q_OBJECT
public:
    DelayedJobCleaner(const QSet<QSharedPointer<FindMatchesJob> > &jobs, const QSet<AbstractRunner *> &runners = QSet<AbstractRunner *>());
    ~DelayedJobCleaner();

private Q_SLOTS:
    void jobDone(ThreadWeaver::JobPointer);
    void checkIfFinished();

private:
    ThreadWeaver::Queue *m_weaver;
    QSet<QSharedPointer<FindMatchesJob> > m_jobs;
    QSet<AbstractRunner *> m_runners;
};

}

#endif // PLASMA_RUNNERJOBS_P_H
