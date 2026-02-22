#include "TwoOptOptimizer.h"

#include <algorithm>

TwoOptOptimizer::TwoOptOptimizer(const Tour& initial, int checksPerIter, uint32_t seed)
: m_checksPerIter(std::max(250, checksPerIter)),
  m_rng(seed),
  m_current(initial),
  m_best(initial),
  m_baseline(initial.cost())
{
}

double TwoOptOptimizer::deltaReverseOpen(const std::vector<TspPoint>& pts,
                                        const std::vector<int>& ord,
                                        int i,
                                        int j)
{
    const int n = static_cast<int>(ord.size());
    if (n < 4) return 0.0;
    if (i < 0 || j < 0 || i >= n || j >= n) return 0.0;
    if (j - i <= 1) return 0.0;

    // Full reversal in a symmetric metric is a no-op.
    if (i == 0 && j == n - 1) return 0.0;

    double delta = 0.0;

    if (i > 0)
    {
        const int a = ord[i - 1];
        const int b = ord[i];
        const int c = ord[j];
        delta += Tour::edgeCost(pts[a], pts[c]) - Tour::edgeCost(pts[a], pts[b]);
    }

    if (j < n - 1)
    {
        const int b = ord[i];
        const int c = ord[j];
        const int d = ord[j + 1];
        delta += Tour::edgeCost(pts[b], pts[d]) - Tour::edgeCost(pts[c], pts[d]);
    }

    return delta;
}

bool TwoOptOptimizer::iterate()
{
    const int n = m_current.size();
    if (n < 4) return false;

    const auto* inst = m_current.instance();
    if (!inst) return false;

    const auto& pts = inst->points();
    auto& ord = m_current.order();

    std::uniform_int_distribution<int> pick(0, n - 1);

    double bestDelta = 0.0;
    int bestI = -1;
    int bestJ = -1;

    for (int t = 0; t < m_checksPerIter; ++t)
    {
        int i = pick(m_rng);
        int j = pick(m_rng);
        if (i == j) continue;
        if (i > j) std::swap(i, j);
        if (j - i <= 1) continue;

        const double delta = deltaReverseOpen(pts, ord, i, j);
        if (delta < bestDelta)
        {
            bestDelta = delta;
            bestI = i;
            bestJ = j;
        }
    }

    if (bestI >= 0)
    {
        std::reverse(ord.begin() + bestI, ord.begin() + bestJ + 1);
        m_current.evaluate();

        if (m_current.cost() < m_best.cost())
        {
            m_best = m_current;
            return true;
        }
    }

    return false;
}
