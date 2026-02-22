#include "ArqOptimizer.h"
#include "../TspInstance.h"
#include <algorithm>
#include <cmath>

static inline double clamp01(double x)
{
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

ArqOptimizer::ArqOptimizer(const Tour& initial, int populationSize, uint32_t seed)
: m_instance(initial.instance()),
  m_n(initial.size()),
  m_popSize(std::max(4, populationSize)),
  m_rng(seed),
  m_best(initial),
  m_baseline(initial.cost()),
  m_lastBest(initial.cost())
{
    if (!m_instance || m_n < 2)
        return;

    m_pop.reserve(m_popSize);
    m_cost.reserve(m_popSize);

    // Seed the population from the initial tour but diversified.
    // Keep node 0 fixed at position 0 for symmetry reduction.
    std::vector<int> base = initial.order();
    if (!base.empty() && base[0] != 0)
    {
        auto it = std::find(base.begin(), base.end(), 0);
        if (it != base.end())
            std::swap(base[0], *it);
    }

    for (int i = 0; i < m_popSize; ++i)
    {
        std::vector<int> ord = base;
        // increase swaps with i for better spread
        const int swaps = std::min(m_n * 2, 2000 + i * 50);
        randomizeOrder(ord, swaps);

        const double c = costOf(ord);
        m_pop.push_back(std::move(ord));
        m_cost.push_back(c);

        if (c < m_lastBest)
        {
            m_best = Tour(m_instance, m_pop.back());
            m_lastBest = c;
        }
    }

    beginGeneration();
}

double ArqOptimizer::costOf(const std::vector<int>& ord) const
{
    if (!m_instance || ord.size() < 2) return 0.0;
    const auto& pts = m_instance->points();
    double sum = 0.0;
    for (int i = 0; i < static_cast<int>(ord.size()) - 1; ++i)
        sum += Tour::edgeCost(pts[ord[i]], pts[ord[i + 1]]);
    return sum;
}

std::vector<int> ArqOptimizer::randomTourOrder()
{
    std::vector<int> ord(m_n);
    for (int i = 0; i < m_n; ++i) ord[i] = i;
    if (m_n > 2)
        std::shuffle(ord.begin() + 1, ord.end(), m_rng); // keep 0 fixed
    return ord;
}

void ArqOptimizer::randomizeOrder(std::vector<int>& ord, int swaps)
{
    if (m_n < 3) return;
    std::uniform_int_distribution<int> dist(1, m_n - 1); // keep 0 fixed
    for (int s = 0; s < swaps; ++s)
    {
        const int a = dist(m_rng);
        const int b = dist(m_rng);
        std::swap(ord[a], ord[b]);
    }
}

int ArqOptimizer::pickDistinctIndex(int avoid1, int avoid2)
{
    std::uniform_int_distribution<int> dist(0, m_popSize - 1);
    int r = 0;
    do {
        r = dist(m_rng);
    } while (r == avoid1 || r == avoid2);
    return r;
}

int ArqOptimizer::pickPBestIndex()
{
    const int P = std::max(2, static_cast<int>(std::ceil(m_pbest * m_popSize)));
    std::uniform_int_distribution<int> dist(0, P - 1);
    return m_rank[dist(m_rng)];
}

double ArqOptimizer::sampleF()
{
    // cauchy around muF (like JADE); resample until in [Flo, Fhi]
    std::cauchy_distribution<double> cauchy(m_muF, 0.10);
    double F = 0.0;
    for (int tries = 0; tries < 32; ++tries)
    {
        F = cauchy(m_rng);
        if (std::isfinite(F) && F >= m_Flo && F <= m_Fhi)
            break;
    }
    if (!std::isfinite(F)) F = m_muF;
    if (F < m_Flo) F = m_Flo;
    if (F > m_Fhi) F = m_Fhi;
    return F;
}

double ArqOptimizer::sampleCR()
{
    std::normal_distribution<double> norm(m_muCR, 0.10);
    double CR = norm(m_rng);
    if (!std::isfinite(CR)) CR = m_muCR;
    return clamp01(CR);
}

std::vector<int> ArqOptimizer::orderCrossover(const std::vector<int>& a, const std::vector<int>& b, double CR)
{
    // OX-like crossover that keeps 0 fixed at position 0 and operates on positions [1..n-1].
    // Segment length roughly CR*(n-1), but capped for very large n to keep the step lightweight.
    const int n = m_n;
    if (n < 4) return a;

    const int maxSeg = std::min(n - 1, 800); // cap to avoid huge copy on large n
    int segLen = static_cast<int>(std::round(CR * static_cast<double>(n - 1)));
    segLen = std::max(10, std::min(maxSeg, segLen));

    std::uniform_int_distribution<int> startDist(1, (n - 1) - segLen + 1);
    const int start = startDist(m_rng);
    const int end = start + segLen - 1;

    std::vector<int> child(n, -1);
    child[0] = 0;

    std::vector<char> used(static_cast<size_t>(n), 0);
    used[0] = 1;

    // copy segment from b
    for (int i = start; i <= end; ++i)
    {
        child[i] = b[i];
        used[static_cast<size_t>(b[i])] = 1;
    }

    // fill remaining positions from a in order (OX wrap-around)
    int writePos = end + 1;
    if (writePos >= n) writePos = 1;

    for (int i = 1; i < n; ++i)
    {
        const int v = a[i];
        if (used[static_cast<size_t>(v)]) continue;

        // skip the protected segment
        while (writePos >= start && writePos <= end)
        {
            writePos = end + 1;
            if (writePos >= n) writePos = 1;
        }

        child[writePos] = v;
        used[static_cast<size_t>(v)] = 1;

        ++writePos;
        if (writePos >= n) writePos = 1;
    }

    // any holes (shouldn't happen) fill with missing ids
    for (int i = 1; i < n; ++i)
    {
        if (child[i] != -1) continue;
        for (int v = 1; v < n; ++v)
        {
            if (!used[static_cast<size_t>(v)])
            {
                child[i] = v;
                used[static_cast<size_t>(v)] = 1;
                break;
            }
        }
    }

    return child;
}

void ArqOptimizer::applyDifferenceToward(std::vector<int>& trial, const std::vector<int>& donor, double F)
{
    // Move a subset of donor positions into the trial by swaps (keeps permutation valid).
    // F controls how many positions are enforced.
    const int n = m_n;
    if (n < 4) return;

    // normalize F into [0,1] strength (F in [Flo, Fhi])
    const double strength = clamp01(F / m_Fhi);

    // Build position map for trial
    std::vector<int> pos(static_cast<size_t>(n), -1);
    for (int i = 0; i < n; ++i)
        pos[static_cast<size_t>(trial[i])] = i;

    // collect differing positions (excluding 0)
    std::vector<int> diff;
    diff.reserve(static_cast<size_t>(n / 2));
    for (int i = 1; i < n; ++i)
        if (trial[i] != donor[i]) diff.push_back(i);

    if (diff.empty()) return;

    int m = static_cast<int>(std::round(strength * static_cast<double>(diff.size())));
    m = std::max(1, std::min(static_cast<int>(diff.size()), std::min(600, m))); // cap effort

    std::shuffle(diff.begin(), diff.end(), m_rng);

    for (int t = 0; t < m; ++t)
    {
        const int p = diff[t];
        const int val = donor[p];

        const int curPos = pos[static_cast<size_t>(val)];
        if (curPos == p) continue;

        std::swap(trial[p], trial[curPos]);
        pos[static_cast<size_t>(trial[curPos])] = curPos;
        pos[static_cast<size_t>(trial[p])] = p;
    }
}

void ArqOptimizer::smallPerturbation(std::vector<int>& ord)
{
    // occasional 2-opt-like reversal on a small segment (excluding position 0)
    if (m_n < 6) return;

    std::uniform_int_distribution<int> dist(1, m_n - 1);
    int a = dist(m_rng);
    int b = dist(m_rng);
    if (a == b) return;
    if (a > b) std::swap(a, b);
    if (b - a < 3) return;

    std::reverse(ord.begin() + a, ord.begin() + b + 1);
}

void ArqOptimizer::archivePush(const std::vector<int>& ord)
{
    m_archive.push_back(ord);
}

void ArqOptimizer::archiveTrim()
{
    const int cap = std::max(1, static_cast<int>(std::round(m_archiveRate * m_popSize)));
    if (static_cast<int>(m_archive.size()) <= cap) return;

    // keep the most recent cap entries
    const int start = static_cast<int>(m_archive.size()) - cap;
    m_archive.erase(m_archive.begin(), m_archive.begin() + start);
}

void ArqOptimizer::beginGeneration()
{
    // update ranking
    m_rank.resize(m_popSize);
    for (int i = 0; i < m_popSize; ++i) m_rank[i] = i;

    std::sort(m_rank.begin(), m_rank.end(),
              [&](int a, int b){ return m_cost[a] < m_cost[b]; });

    m_SF.clear();
    m_SCR.clear();
    m_SG.clear();
}

void ArqOptimizer::endGeneration()
{
    // update muF, muCR from successes (JADE-like)
    if (!m_SF.empty())
    {
        double sumW = 0.0;
        for (double g : m_SG) sumW += g;
        if (sumW <= 0.0) sumW = static_cast<double>(m_SG.size());

        double meanCR = 0.0;
        double numF = 0.0, denF = 0.0;

        for (size_t i = 0; i < m_SF.size(); ++i)
        {
            const double w = (m_SG[i] > 0.0) ? (m_SG[i] / sumW) : (1.0 / static_cast<double>(m_SF.size()));
            meanCR += w * m_SCR[i];

            numF += w * (m_SF[i] * m_SF[i]);
            denF += w * m_SF[i];
        }

        const double lehmerF = (denF > 0.0) ? (numF / denF) : m_muF;

        m_muCR = (1.0 - m_shc) * m_muCR + m_shc * clamp01(meanCR);
        m_muF  = (1.0 - m_shc) * m_muF  + m_shc * lehmerF;

        // sanitize
        if (m_muF < m_Flo) m_muF = m_Flo;
        if (m_muF > m_Fhi) m_muF = m_Fhi;
    }

    // stagnation logic on generation boundary
    if (m_lastBest + 1e-12 < m_bestPrev)
    {
        m_bestPrev = m_lastBest;
        m_noImproveGen = 0;
    }
    else
    {
        m_noImproveGen++;
        if (m_noImproveGen >= m_stagTrigger)
        {
            restartWorst();
            m_noImproveGen = 0;
            m_bestPrev = m_lastBest;
        }
    }

    archiveTrim();
}

void ArqOptimizer::restartWorst()
{
    // restart a fraction of the worst solutions around the current best
    const int W = std::max(1, static_cast<int>(std::round(m_worstFrac * m_popSize)));

    // ensure ranking exists
    if (m_rank.empty())
        beginGeneration();

    const std::vector<int> bestOrd = m_pop[m_rank[0]];

    for (int k = 0; k < W; ++k)
    {
        const int idx = m_rank[m_popSize - 1 - k];
        std::vector<int> ord = bestOrd;

        // rsigma controls swaps, capped to keep step reasonable
        int swaps = static_cast<int>(std::round(m_rsigma * static_cast<double>(m_n)));
        swaps = std::max(50, std::min(1200, swaps));
        randomizeOrder(ord, swaps);

        const double c = costOf(ord);
        m_pop[idx] = std::move(ord);
        m_cost[idx] = c;

        if (c < m_lastBest)
        {
            m_best = Tour(m_instance, m_pop[idx]);
            m_lastBest = c;
        }
    }
}

bool ArqOptimizer::iterate()
{
    if (!m_instance || m_n < 2 || m_pop.empty())
        return false;

    bool improved = false;

    // generation boundary bookkeeping
    if (m_target == 0)
        beginGeneration();

    const int i = m_target;

    // select guidance and donors
    const int pbestIdx = pickPBestIndex();
    const int r1 = pickDistinctIndex(i, pbestIdx);

    // r2 can be from archive with some probability
    const bool useArchive = (!m_archive.empty() && (std::uniform_real_distribution<double>(0.0,1.0)(m_rng) < 0.35));

    const std::vector<int>& parent = m_pop[i];
    const std::vector<int>& pbest  = m_pop[pbestIdx];
    const std::vector<int>& donor1 = m_pop[r1];

    std::vector<int> donor2;
    if (useArchive)
    {
        std::uniform_int_distribution<int> d(0, static_cast<int>(m_archive.size()) - 1);
        donor2 = m_archive[d(m_rng)];
    }
    else
    {
        const int r2 = pickDistinctIndex(i, r1);
        donor2 = m_pop[r2];
    }

    // sample control parameters
    const double F  = sampleF();
    const double CR = sampleCR();

    // trial generation (permutation "DE-like")
    std::vector<int> trial = orderCrossover(parent, pbest, CR);

    // push trial toward donor2 using difference operations weighted by F
    applyDifferenceToward(trial, donor2, F);

    // occasional small perturbation to escape local traps
    if (std::uniform_real_distribution<double>(0.0,1.0)(m_rng) < 0.10)
        smallPerturbation(trial);

    // evaluate and select
    const double f_parent = m_cost[i];
    const double f_trial  = costOf(trial);

    if (f_trial < f_parent)
    {
        archivePush(parent);

        m_pop[i] = std::move(trial);
        m_cost[i] = f_trial;

        // record success
        m_SF.push_back(F);
        m_SCR.push_back(CR);
        m_SG.push_back(f_parent - f_trial);

        if (f_trial < m_lastBest)
        {
            m_best = Tour(m_instance, m_pop[i]);
            m_lastBest = f_trial;
            improved = true;
        }
    }

    // advance target
    m_target++;
    if (m_target >= m_popSize)
    {
        m_target = 0;
        endGeneration();
    }

    return improved;
}
