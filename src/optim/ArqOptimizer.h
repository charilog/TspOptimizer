#pragma once

#include "IOptimizer.h"
#include <random>
#include <vector>
#include <limits>

// ARQ (TSP adaptation).
// The attached "optimsolution::ARQ" code is for continuous problems and depends on the optimsolution framework.
// This implementation ports the core idea to TSP permutations:
// - population-based search
// - p-best guidance
// - adaptive parameters (muF, muCR) similar to JADE/L-SHADE
// - archive of replaced solutions
// - stagnation-triggered restart of the worst fraction
class ArqOptimizer final : public IOptimizer
{
public:
    ArqOptimizer(const Tour& initial,
                 int populationSize = 30,
                 uint32_t seed = std::random_device{}());

    bool iterate() override;
    const Tour& bestTour() const override { return m_best; }
    double baselineCost() const override { return m_baseline; }

private:
    double costOf(const std::vector<int>& ord) const;

    std::vector<int> randomTourOrder();
    void randomizeOrder(std::vector<int>& ord, int swaps);

    // permutation operators
    std::vector<int> orderCrossover(const std::vector<int>& a, const std::vector<int>& b, double CR);
    void applyDifferenceToward(std::vector<int>& trial, const std::vector<int>& donor, double F);
    void smallPerturbation(std::vector<int>& ord);

    // policy / adaptation
    void beginGeneration();
    void endGeneration();
    double sampleF();
    double sampleCR();

    int pickDistinctIndex(int avoid1, int avoid2 = -1);
    int pickPBestIndex();

    void archivePush(const std::vector<int>& ord);
    void archiveTrim();

    void restartWorst();

private:
    const TspInstance* m_instance = nullptr;
    int m_n = 0;

    // parameters (paper-like defaults)
    int m_popSize = 30;

    double m_pbest = 0.12;
    double m_muF   = 0.60;
    double m_muCR  = 0.85;

    double m_Flo = 0.05;
    double m_Fhi = 1.40;

    double m_worstFrac = 0.08;
    double m_rsigma = 0.18;
    int    m_stagTrigger = 24;

    double m_shc = 0.10;         // smoothing constant for mu updates
    double m_archiveRate = 1.5;  // archive capacity = rate * pop

    // state
    std::mt19937 m_rng;

    std::vector<std::vector<int>> m_pop;
    std::vector<double> m_cost;

    std::vector<int> m_rank; // indices sorted by cost ascending (updated each generation)

    std::vector<std::vector<int>> m_archive;

    // asynchronous stepping (one target per iterate())
    int m_target = 0;
    int m_noImproveGen = 0;
    double m_bestPrev = std::numeric_limits<double>::infinity();

    // success history in current generation
    std::vector<double> m_SF;
    std::vector<double> m_SCR;
    std::vector<double> m_SG;

    // global best
    Tour m_best;
    double m_baseline = 0.0;
    double m_lastBest = 0.0;
};
