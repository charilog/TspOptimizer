#pragma once

#include <QObject>
#include <QVector>
#include <atomic>
#include <memory>

#include "IOptimizer.h"

class OptimizerWorker : public QObject
{
    Q_OBJECT
public:
    explicit OptimizerWorker(std::unique_ptr<IOptimizer> optimizer, QObject* parent = nullptr);

public slots:
    void run();
    void stop();

signals:
    void bestUpdated(QVector<int> bestOrder, double bestCost, double improvementPercent);
    void finished();

private:
    std::unique_ptr<IOptimizer> m_optimizer;
    std::atomic_bool m_running { true };
};
