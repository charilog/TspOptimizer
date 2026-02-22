#include "OptimizerWorker.h"
#include <QThread>

OptimizerWorker::OptimizerWorker(std::unique_ptr<IOptimizer> optimizer, QObject* parent)
: QObject(parent), m_optimizer(std::move(optimizer))
{
}

void OptimizerWorker::stop()
{
    m_running.store(false, std::memory_order_relaxed);
}

void OptimizerWorker::run()
{
    if (!m_optimizer)
    {
        emit finished();
        return;
    }

    const double baseline = m_optimizer->baselineCost();

    while (m_running.load(std::memory_order_relaxed))
    {
        const bool improved = m_optimizer->iterate();
        if (improved)
        {
            const Tour& best = m_optimizer->bestTour();
            QVector<int> ord;
            ord.reserve(best.size());
            for (int v : best.order()) ord.push_back(v);

            const double bestCost = best.cost();
            const double pct = (baseline > 0.0) ? ((baseline - bestCost) / baseline * 100.0) : 0.0;

            emit bestUpdated(ord, bestCost, pct);
        }

        // Yield a bit so the GUI stays responsive even on single-core systems.
        QThread::yieldCurrentThread();
    }

    emit finished();
}
