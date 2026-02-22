#pragma once

#include "TspInstance.h"
#include <vector>
#include <random>
#include <cstdint>
#include <cstdlib>

class Tour
{
public:
    Tour() = default;
    explicit Tour(const TspInstance* instance);             // identity order
    Tour(const TspInstance* instance, std::vector<int> ord);

    const TspInstance* instance() const { return m_instance; }
    const std::vector<int>& order() const { return m_order; }
    std::vector<int>& order() { return m_order; }

    double cost() const { return m_cost; }
    double evaluate(); // recompute cost

    int size() const { return static_cast<int>(m_order.size()); }

    // Helpers (same ideas as the Java app)
    void randomize(int swaps, std::mt19937& rng);
    void easyHeuristic();     // insertion heuristic (fast)
    void thoroughHeuristic(); // distance-from-center sorting + insertion

    // Mutations
    void mutateSwap(std::mt19937& rng);           // swap 2 indices (excluding 0 like Java)
    void mutateInsertion(std::mt19937& rng);      // remove/insert
    void mutateReverseSegment(std::mt19937& rng); // reverse a subsegment (2-opt style)

    // Utility
    static inline double edgeCost(const TspPoint& a, const TspPoint& b)
    {
        const int32_t dx = std::abs(a.x - b.x);
        const int32_t dy = std::abs(a.y - b.y);
        return static_cast<double>(dx > dy ? dx : dy); // Chebyshev distance (same as Java)
    }

private:
    const TspInstance* m_instance = nullptr;
    std::vector<int> m_order;
    double m_cost = 0.0;
};
