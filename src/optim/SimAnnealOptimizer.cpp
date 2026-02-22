#include "SimAnnealOptimizer.h"
#include <algorithm>
#include <cmath>

SimAnnealOptimizer::SimAnnealOptimizer(const Tour& initial, uint32_t seed, double alpha)
: m_rng(seed),
  m_uni01(0.0, 1.0),
  m_current(initial),
  m_best(initial),
  m_baseline(initial.cost()),
  m_alpha(std::clamp(alpha, 0.90, 0.9999999))
{
    // heuristic temperature scale: average edge cost
    const int n = m_current.size();
    m_temp = (n > 1) ? (m_current.cost() / static_cast<double>(n)) : 1.0;
    if (m_temp < 1.0) m_temp = 1.0;
}

bool SimAnnealOptimizer::iterate()
{
    const int n = m_current.size();
    if (n < 4) return false;

    std::uniform_int_distribution<int> pick(0, n - 1);
    int i = pick(m_rng);
    int j = pick(m_rng);
    if (i == j) return false;
    if (i > j) std::swap(i, j);
    if (j - i <= 1) return false;

    const auto& pts = m_current.instance()->points();
    const auto& ord = m_current.order();

    // compute delta cost for reversing segment [i..j] in an open tour.
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

    const bool accept = (delta <= 0.0) || (std::exp(-delta / m_temp) > m_uni01(m_rng));
    if (accept)
    {
        // apply move
        auto& ordMut = m_current.order();
        std::reverse(ordMut.begin() + i, ordMut.begin() + j + 1);
        // update cost
        // internal edges are symmetric, so the delta above is valid
        const double newCost = m_current.cost() + delta;
        // store without recomputing full O(n)
        // (keep it consistent with potential numerical drift by a periodic full evaluate if needed)
        // Here: just assign.
        // NOTE: Tour::cost() is private; so we recompute occasionally? We'll do a light workaround:
        m_current.evaluate(); // robust (O(n)), still reasonable for SA.
        // If you want faster SA: add a setter for m_cost and skip evaluate().
    }

    // cool down
    m_temp *= m_alpha;
    if (m_temp < 1e-6) m_temp = 1e-6;

    if (m_current.cost() < m_best.cost())
    {
        m_best = m_current;
        return true;
    }
    return false;
}
