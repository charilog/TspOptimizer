#include "AcoOptimizer.h"
#include "../TspInstance.h"
#include <algorithm>
#include <cmath>

static inline int clampInt(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

AcoOptimizer::AcoOptimizer(const Tour& initial,
                           int antsPerIteration,
                           int candidateK,
                           int candidateSamples,
                           double alpha,
                           double beta,
                           double rho,
                           double q,
                           uint32_t seed)
: m_instance(initial.instance()),
  m_n(initial.size()),
  m_antsPerIter(std::max(1, antsPerIteration)),
  m_candidateK(std::max(4, candidateK)),
  m_candidateSamples(std::max(10, candidateSamples)),
  m_alpha(alpha),
  m_beta(beta),
  m_rho(rho),
  m_Q(q),
  m_rng(seed),
  m_best(initial),
  m_baseline(initial.cost()),
  m_lastBest(initial.cost())
{
    if (m_instance && m_n > 1)
        buildCandidateLists();
}

double AcoOptimizer::costOf(const std::vector<int>& ord) const
{
    if (!m_instance || ord.size() < 2) return 0.0;
    const auto& pts = m_instance->points();

    double sum = 0.0;
    for (int i = 0; i < static_cast<int>(ord.size()) - 1; ++i)
        sum += Tour::edgeCost(pts[ord[i]], pts[ord[i + 1]]);
    return sum;
}

void AcoOptimizer::buildCandidateLists()
{
    m_candidates.clear();
    m_tau.clear();

    if (!m_instance || m_n <= 1)
        return;

    const auto& pts = m_instance->points();

    // For very large instances, avoid O(N^2) neighbor building.
    // We approximate k-nearest neighbors by random sampling per node.
    const int K = clampInt(m_candidateK, 4, std::max(4, m_n - 1));
    const int S = std::min(m_candidateSamples, std::max(10, m_n - 1));

    std::uniform_int_distribution<int> pick(0, m_n - 1);

    m_candidates.resize(m_n);
    m_tau.resize(m_n);

    for (int i = 0; i < m_n; ++i)
    {
        std::vector<std::pair<double,int>> best;
        best.reserve(static_cast<size_t>(K + 8));

        // sample S distinct-ish nodes (duplicates filtered cheaply)
        for (int s = 0; s < S; ++s)
        {
            int j = pick(m_rng);
            if (j == i) continue;

            const double d = Tour::edgeCost(pts[i], pts[j]);

            // keep the best K by distance
            if (static_cast<int>(best.size()) < K)
            {
                best.emplace_back(d, j);
                continue;
            }

            // find current worst
            int worstIdx = 0;
            for (int t = 1; t < static_cast<int>(best.size()); ++t)
                if (best[t].first > best[worstIdx].first) worstIdx = t;

            if (d < best[worstIdx].first)
                best[worstIdx] = {d, j};
        }

        // sort ascending by distance
        std::sort(best.begin(), best.end(),
                  [](const auto& a, const auto& b){ return a.first < b.first; });

        // unique ids
        std::vector<int> cand;
        cand.reserve(K);
        for (const auto& p : best)
        {
            if (static_cast<int>(cand.size()) >= K) break;
            const int id = p.second;
            if (id == i) continue;
            bool dup = false;
            for (int x : cand) { if (x == id) { dup = true; break; } }
            if (!dup) cand.push_back(id);
        }

        // if not enough (duplicates), fill randomly
        while (static_cast<int>(cand.size()) < K)
        {
            int j = pick(m_rng);
            if (j == i) continue;
            bool dup = false;
            for (int x : cand) { if (x == j) { dup = true; break; } }
            if (!dup) cand.push_back(j);
        }

        m_candidates[i] = std::move(cand);
        m_tau[i].assign(static_cast<size_t>(K), 1.0); // tau0
    }

    // reset iteration state
    m_antIndex = 0;
    m_iterBestCost = std::numeric_limits<double>::infinity();
    m_iterBestOrder.clear();
}

int AcoOptimizer::pickRandomUnvisited(const std::vector<char>& visited)
{
    std::uniform_int_distribution<int> pick(0, m_n - 1);
    for (int tries = 0; tries < 1024; ++tries)
    {
        const int j = pick(m_rng);
        if (!visited[j]) return j;
    }
    // fallback linear
    for (int j = 0; j < m_n; ++j)
        if (!visited[j]) return j;
    return 0;
}

int AcoOptimizer::chooseNext(int current, const std::vector<char>& visited)
{
    const auto& pts = m_instance->points();

    const auto& cand = m_candidates[current];
    const auto& tau  = m_tau[current];

    // Compute desirabilities for unvisited candidates
    std::vector<double> w;
    w.resize(cand.size(), 0.0);

    double sumW = 0.0;
    for (size_t k = 0; k < cand.size(); ++k)
    {
        const int j = cand[k];
        if (visited[j]) continue;

        const double d = Tour::edgeCost(pts[current], pts[j]);
        const double eta = 1.0 / (1.0 + d); // heuristic

        const double t = std::max(1e-12, tau[k]);
        const double val = std::pow(t, m_alpha) * std::pow(eta, m_beta);
        w[k] = val;
        sumW += val;
    }

    if (sumW <= 0.0)
        return pickRandomUnvisited(visited);

    std::uniform_real_distribution<double> uni01(0.0, 1.0);
    const double r = uni01(m_rng) * sumW;

    double acc = 0.0;
    for (size_t k = 0; k < cand.size(); ++k)
    {
        if (w[k] <= 0.0) continue;
        acc += w[k];
        if (acc >= r)
            return cand[k];
    }

    // numeric fallback
    for (size_t k = 0; k < cand.size(); ++k)
        if (w[k] > 0.0) return cand[k];

    return pickRandomUnvisited(visited);
}

std::vector<int> AcoOptimizer::constructTour()
{
    std::vector<int> ord;
    ord.reserve(static_cast<size_t>(m_n));

    std::vector<char> visited(static_cast<size_t>(m_n), 0);

    // Fix node 0 as start (removes symmetry and matches other optimizers' behavior)
    int current = 0;
    ord.push_back(current);
    visited[current] = 1;

    for (int step = 1; step < m_n; ++step)
    {
        const int nxt = chooseNext(current, visited);
        ord.push_back(nxt);
        visited[nxt] = 1;
        current = nxt;
    }

    return ord;
}

bool AcoOptimizer::iterate()
{
    if (!m_instance || m_n < 2 || m_candidates.empty() || m_tau.empty())
        return false;

    bool improved = false;

    // Build ONE ant tour per iterate() call
    std::vector<int> ord = constructTour();
    const double c = costOf(ord);

    if (c < m_iterBestCost)
    {
        m_iterBestCost = c;
        m_iterBestOrder = ord;
    }

    if (c < m_lastBest)
    {
        m_best = Tour(m_instance, std::move(ord));
        m_lastBest = m_best.cost();
        improved = true;
    }

    ++m_antIndex;

    // After all ants of this "iteration", update pheromones using the iteration-best tour.
    if (m_antIndex >= m_antsPerIter)
    {
        // evaporation
        const double evap = (m_rho < 0.0) ? 0.0 : (m_rho > 1.0 ? 1.0 : m_rho);
        const double keep = 1.0 - evap;

        for (int i = 0; i < m_n; ++i)
            for (double& t : m_tau[i])
                t *= keep;

        // deposit from best tour of the batch
        if (!m_iterBestOrder.empty() && std::isfinite(m_iterBestCost) && m_iterBestCost > 0.0)
        {
            const double delta = m_Q / m_iterBestCost;

            for (int i = 0; i < m_n - 1; ++i)
            {
                const int a = m_iterBestOrder[i];
                const int b = m_iterBestOrder[i + 1];

                // update tau[a][k] where candidates[a][k] == b
                auto& candA = m_candidates[a];
                auto& tauA  = m_tau[a];
                for (size_t k = 0; k < candA.size(); ++k)
                {
                    if (candA[k] == b) { tauA[k] += delta; break; }
                }

                // also update reverse direction if present (helps symmetry)
                auto& candB = m_candidates[b];
                auto& tauB  = m_tau[b];
                for (size_t k = 0; k < candB.size(); ++k)
                {
                    if (candB[k] == a) { tauB[k] += delta; break; }
                }
            }
        }

        // reset batch
        m_antIndex = 0;
        m_iterBestCost = std::numeric_limits<double>::infinity();
        m_iterBestOrder.clear();
    }

    return improved;
}
