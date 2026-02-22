#pragma once

#include "IOptimizer.h"
#include <vector>
#include <random>

class GeneticOptimizer final : public IOptimizer
{
public:
    GeneticOptimizer(const Tour& initial,
                     int populationSize = 30,
                     int mutationRate = 2,
                     uint32_t seed = std::random_device{}());

    bool iterate() override;
    const Tour& bestTour() const override { return m_best; }
    double baselineCost() const override { return m_baseline; }

private:
    int m_populationSize = 30;
    int m_mutationRate = 2;

    std::mt19937 m_rng;

    std::vector<Tour> m_population;
    Tour m_best;
    double m_baseline = 0.0;
    double m_lastBest = 0.0;
};
