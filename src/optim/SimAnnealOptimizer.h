#pragma once

#include "IOptimizer.h"
#include <random>

class SimAnnealOptimizer final : public IOptimizer
{
public:
    SimAnnealOptimizer(const Tour& initial,
                       uint32_t seed = std::random_device{}(),
                       double alpha = 0.999995);

    bool iterate() override;
    const Tour& bestTour() const override { return m_best; }
    double baselineCost() const override { return m_baseline; }

private:
    std::mt19937 m_rng;
    std::uniform_real_distribution<double> m_uni01;

    Tour m_current;
    Tour m_best;

    double m_baseline = 0.0;
    double m_temp = 1.0;
    double m_alpha = 0.999995;
};
