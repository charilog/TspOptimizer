#include "IlsOptimizer.h"

#include <algorithm>

IlsOptimizer::IlsOptimizer(const Tour& initial, int checksPerIter, int stagnationIters, uint32_t seed)
: m_checksPerIter(std::max(250, checksPerIter)),
  m_stagnationIters(std::max(10, stagnationIters)),
  m_noImprove(0),
  m_rng(seed),
  m_current(initial),
  m_best(initial),
  m_baseline(initial.cost())
{
}

double IlsOptimizer::deltaReverseOpen(const std::vector<TspPoint>& pts,
                                     const std::vector<int>& ord,
                                     int i,
                                     int j)
{
    const int n = static_cast<int>(ord.size());
    if (n < 4) return 0.0;
    if (i < 0 || j < 0 || i >= n || j >= n) return 0.0;
    if (j - i <= 1) return 0.0;
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

bool IlsOptimizer::applyBest2OptMove()
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
        return true;
    }

    return false;
}

void IlsOptimizer::doubleBridgePerturbation()
{
    const int n = m_current.size();
    if (n < 8) return;

    auto& ord = m_current.order();

    // Choose 4 cut points i < j < k < l to create 5 segments:
    // A=[0..i-1], B=[i..j-1], C=[j..k-1], D=[k..l-1], E=[l..n-1]
    // New order: A + C + B + D + E  (classic double-bridge style for permutations)
    std::uniform_int_distribution<int> di(1, n - 6);
    int i = di(m_rng);

    std::uniform_int_distribution<int> dj(i + 1, n - 5);
    int j = dj(m_rng);

    std::uniform_int_distribution<int> dk(j + 1, n - 4);
    int k = dk(m_rng);

    std::uniform_int_distribution<int> dl(k + 1, n - 2);
    int l = dl(m_rng);

    std::vector<int> newOrd;
    newOrd.reserve(n);

    newOrd.insert(newOrd.end(), ord.begin(), ord.begin() + i);       // A
    newOrd.insert(newOrd.end(), ord.begin() + j, ord.begin() + k);   // C
    newOrd.insert(newOrd.end(), ord.begin() + i, ord.begin() + j);   // B
    newOrd.insert(newOrd.end(), ord.begin() + k, ord.begin() + l);   // D
    newOrd.insert(newOrd.end(), ord.begin() + l, ord.end());         // E

    ord = std::move(newOrd);
}

bool IlsOptimizer::iterate()
{
    const int n = m_current.size();
    if (n < 4) return false;

    bool improvedBest = false;

    if (applyBest2OptMove())
    {
        m_noImprove = 0;
        if (m_current.cost() < m_best.cost())
        {
            m_best = m_current;
            improvedBest = true;
        }
    }
    else
    {
        ++m_noImprove;
        if (m_noImprove >= m_stagnationIters)
        {
            doubleBridgePerturbation();
            m_current.evaluate();
            m_noImprove = 0;

            if (m_current.cost() < m_best.cost())
            {
                m_best = m_current;
                improvedBest = true;
            }
        }
    }

    return improvedBest;
}
