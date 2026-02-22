#pragma once

#include "IOptimizer.h"
#include <random>
#include <vector>
#include <limits>

// Ant Colony Optimization (sparse candidate-list variant suitable for large TSP instances).
// Notes:
// - Uses a per-node candidate list of size K (approximate nearest neighbors by random sampling).
// - Maintains pheromone only on candidate edges (N*K storage).
// - Builds open tours (no return edge), consistent with Tour::evaluate().
class AcoOptimizer final : public IOptimizer
{
public:
    AcoOptimizer(const Tour& initial,
                 int antsPerIteration = 20,
                 int candidateK = 20,
                 int candidateSamples = 200,
                 double alpha = 1.0,
                 double beta = 3.0,
                 double rho = 0.10,
                 double q = 1.0,
                 uint32_t seed = std::random_device{}());

    bool iterate() override;
    const Tour& bestTour() const override { return m_best; }
    double baselineCost() const override { return m_baseline; }

private:
    void buildCandidateLists();
    std::vector<int> constructTour();
    double costOf(const std::vector<int>& ord) const;

    int pickRandomUnvisited(const std::vector<char>& visited);
    int chooseNext(int current, const std::vector<char>& visited);

private:
    const TspInstance* m_instance = nullptr;
    int m_n = 0;

    // Parameters
    int m_antsPerIter = 20;
    int m_candidateK = 20;
    int m_candidateSamples = 200;

    double m_alpha = 1.0;
    double m_beta  = 3.0;
    double m_rho   = 0.10;
    double m_Q     = 1.0;

    // Candidate edges and pheromones: candidates[i][k] with pheromone tau[i][k]
    std::vector<std::vector<int>>    m_candidates;
    std::vector<std::vector<double>> m_tau;

    std::mt19937 m_rng;

    // Iteration bookkeeping (one ant tour per iterate() call)
    int m_antIndex = 0;
    double m_iterBestCost = std::numeric_limits<double>::infinity();
    std::vector<int> m_iterBestOrder;

    // Global best
    Tour m_best;
    double m_baseline = 0.0;
    double m_lastBest = 0.0;
};
