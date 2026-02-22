#pragma once

#include "IOptimizer.h"

#include <random>
#include <vector>

// Classic 2-opt local search (open tour variant).
// Each iterate() samples candidate 2-opt moves and applies the best improving move (if any).
class TwoOptOptimizer final : public IOptimizer
{
public:
    explicit TwoOptOptimizer(const Tour& initial,
                             int checksPerIter = 4000,
                             uint32_t seed = std::random_device{}());

    bool iterate() override;
    const Tour& bestTour() const override { return m_best; }
    double baselineCost() const override { return m_baseline; }

private:
    static double deltaReverseOpen(const std::vector<TspPoint>& pts,
                                   const std::vector<int>& ord,
                                   int i,
                                   int j);

    int m_checksPerIter = 4000;

    std::mt19937 m_rng;

    Tour m_current;
    Tour m_best;
    double m_baseline = 0.0;
};
