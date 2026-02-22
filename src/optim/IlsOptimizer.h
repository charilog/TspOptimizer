#pragma once

#include "IOptimizer.h"

#include <random>
#include <vector>

// Iterated Local Search (ILS) for open TSP tours.
// - Uses 2-opt local improvement.
// - When stagnating, applies a "double-bridge" perturbation to escape local minima.
class IlsOptimizer final : public IOptimizer
{
public:
    explicit IlsOptimizer(const Tour& initial,
                         int checksPerIter = 2500,
                         int stagnationIters = 150,
                         uint32_t seed = std::random_device{}());

    bool iterate() override;
    const Tour& bestTour() const override { return m_best; }
    double baselineCost() const override { return m_baseline; }

private:
    static double deltaReverseOpen(const std::vector<TspPoint>& pts,
                                   const std::vector<int>& ord,
                                   int i,
                                   int j);

    bool applyBest2OptMove();
    void doubleBridgePerturbation();

    int m_checksPerIter = 2500;
    int m_stagnationIters = 150;
    int m_noImprove = 0;

    std::mt19937 m_rng;

    Tour m_current;
    Tour m_best;
    double m_baseline = 0.0;
};
