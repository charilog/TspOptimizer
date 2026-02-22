#include "GeneticOptimizer.h"
#include <algorithm>

GeneticOptimizer::GeneticOptimizer(const Tour& initial, int populationSize, int mutationRate, uint32_t seed)
: m_populationSize(populationSize),
  m_mutationRate(std::max(1, mutationRate)),
  m_rng(seed),
  m_population(),
  m_best(initial),
  m_baseline(initial.cost()),
  m_lastBest(initial.cost())
{
    m_population.reserve(m_populationSize);
    for (int i = 0; i < m_populationSize; ++i)
        m_population.push_back(initial); // copy
}

bool GeneticOptimizer::iterate()
{
    // step 1: rank by fitness (lower is better)
    std::sort(m_population.begin(), m_population.end(),
              [](const Tour& a, const Tour& b){ return a.cost() < b.cost(); });

    m_best = m_population.front();

    // step 2: probabilistic death (similar to Java)
    std::uniform_real_distribution<double> uni01(0.0, 1.0);

    std::vector<Tour> survivors;
    survivors.reserve(m_populationSize);

    survivors.push_back(m_population[0]);
    for (int i = 1; i < m_populationSize; ++i)
    {
        const double pDie = static_cast<double>(i) / static_cast<double>(m_populationSize);
        if (uni01(m_rng) >= pDie)
            survivors.push_back(m_population[i]);
    }

    const int dead = m_populationSize - static_cast<int>(survivors.size());
    if (survivors.empty())
        survivors.push_back(m_population[0]);

    // step 3: repopulate by mutating clones
    std::uniform_int_distribution<int> pickParent(0, static_cast<int>(survivors.size()) - 1);
    std::uniform_int_distribution<int> howManyMut(0, m_mutationRate - 1);
    std::uniform_int_distribution<int> whichMut(0, 2);

    for (int i = 0; i < dead; ++i)
    {
        Tour baby = survivors[pickParent(m_rng)];
        const int k = howManyMut(m_rng);
        for (int j = 0; j < k; ++j)
        {
            switch (whichMut(m_rng))
            {
                case 0: baby.mutateInsertion(m_rng); break;
                case 1: baby.mutateSwap(m_rng); break;
                case 2: baby.mutateReverseSegment(m_rng); break;
            }
        }
        baby.evaluate();
        survivors.push_back(std::move(baby));
    }

    m_population = std::move(survivors);

    // update best
    std::sort(m_population.begin(), m_population.end(),
              [](const Tour& a, const Tour& b){ return a.cost() < b.cost(); });
    const double currentBest = m_population.front().cost();
    if (currentBest < m_lastBest)
    {
        m_best = m_population.front();
        m_lastBest = currentBest;
        return true;
    }
    return false;
}
