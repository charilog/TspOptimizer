#include "Tour.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

Tour::Tour(const TspInstance* instance)
: m_instance(instance)
{
    if (!m_instance)
        throw std::runtime_error("Tour: instance is null");

    m_order.resize(m_instance->size());
    for (int i = 0; i < m_instance->size(); ++i)
        m_order[i] = i;

    evaluate();
}

Tour::Tour(const TspInstance* instance, std::vector<int> ord)
: m_instance(instance), m_order(std::move(ord))
{
    if (!m_instance)
        throw std::runtime_error("Tour: instance is null");
    if (static_cast<int>(m_order.size()) != m_instance->size())
        throw std::runtime_error("Tour: order size does not match instance size");
    evaluate();
}

double Tour::evaluate()
{
    if (!m_instance || m_order.empty())
    {
        m_cost = 0.0;
        return m_cost;
    }

    const auto& pts = m_instance->points();
    double sum = 0.0;
    for (int i = 0; i < static_cast<int>(m_order.size()) - 1; ++i)
    {
        const auto& a = pts[m_order[i]];
        const auto& b = pts[m_order[i + 1]];
        sum += edgeCost(a, b);
    }
    m_cost = sum;
    return m_cost;
}

void Tour::randomize(int swaps, std::mt19937& rng)
{
    if (m_order.size() < 2) return;

    std::uniform_int_distribution<int> dist(0, static_cast<int>(m_order.size()) - 1);
    for (int i = 0; i < swaps; ++i)
    {
        int a = dist(rng);
        int b = dist(rng);
        std::swap(m_order[a], m_order[b]);
    }
    evaluate();
}

void Tour::mutateSwap(std::mt19937& rng)
{
    if (m_order.size() < 3) return;

    std::uniform_int_distribution<int> dist(1, static_cast<int>(m_order.size()) - 2); // exclude 0 and last index, like Java
    int a = dist(rng);
    int b = dist(rng);
    std::swap(m_order[a], m_order[b]);
}

void Tour::mutateInsertion(std::mt19937& rng)
{
    if (m_order.size() < 4) return;

    std::uniform_int_distribution<int> dist(1, static_cast<int>(m_order.size()) - 2);
    int element = dist(rng);
    int insertAfter = dist(rng);
    if (element == insertAfter) return;

    const int val = m_order[element];
    m_order.erase(m_order.begin() + element);

    // If removing shifted indices, fix insertion index.
    if (insertAfter > element) insertAfter -= 1;

    m_order.insert(m_order.begin() + insertAfter + 1, val);
}

void Tour::mutateReverseSegment(std::mt19937& rng)
{
    if (m_order.size() < 4) return;

    std::uniform_int_distribution<int> dist(0, static_cast<int>(m_order.size()) - 1);
    int a = dist(rng);
    int b = dist(rng);
    if (a == b) return;
    int i = std::min(a, b);
    int j = std::max(a, b);
    if (j - i <= 1) return;

    std::reverse(m_order.begin() + i, m_order.begin() + j + 1);
}

void Tour::easyHeuristic()
{
    if (!m_instance || m_order.size() < 3) return;

    const int n = static_cast<int>(m_order.size());
    std::vector<int> newSol(n);

    newSol[0] = m_order[0];
    newSol[1] = m_order[n - 1];

    for (int i = 1; i < n - 1; ++i)
    {
        newSol[i + 1] = newSol[i];
        newSol[i] = m_order[i];

        int bestPos = i;
        // compute partial length for first (i+2) points in newSol using helper below
        auto partialCost = [&](int size)->double{
            const auto& pts = m_instance->points();
            double sum = 0.0;
            for (int k = 0; k < size - 1; ++k)
                sum += edgeCost(pts[newSol[k]], pts[newSol[k+1]]);
            return sum;
        };
        double bestLen = partialCost(i + 2);

        for (int j = i; j > 1; --j)
        {
            std::swap(newSol[j], newSol[j - 1]);
            const double testLen = partialCost(i + 2);
            if (testLen < bestLen)
            {
                bestLen = testLen;
                bestPos = j - 1;
            }
        }

        for (int j = 1; j < bestPos; ++j)
            std::swap(newSol[j], newSol[j + 1]);
    }

    m_order = std::move(newSol);
    evaluate();
}

void Tour::thoroughHeuristic()
{
    if (!m_instance || m_order.size() < 3) return;

    const auto& pts = m_instance->points();
    const int n = static_cast<int>(m_order.size());

    // compute bounding box (same intent as Java)
    int32_t maxX = pts[0].x, maxY = pts[0].y, minX = pts[0].x, minY = pts[0].y;
    for (const auto& p : pts)
    {
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
    }

    const double cx = (static_cast<double>(maxX) - static_cast<double>(minX)) / 2.0;
    const double cy = (static_cast<double>(maxY) - static_cast<double>(minY)) / 2.0;

    // compute distance-from-center for each node id and sort ids by descending distance
    std::vector<int> ids(n);
    for (int i = 0; i < n; ++i) ids[i] = m_order[i];

    std::vector<double> dist(n);
    for (int i = 0; i < n; ++i)
    {
        const double dx = static_cast<double>(pts[ids[i]].x) - cx;
        const double dy = static_cast<double>(pts[ids[i]].y) - cy;
        dist[i] = std::sqrt(dx*dx + dy*dy);
    }

    // bubble sort (to keep behavior close to Java)
    for (int i = 0; i < n - 1; ++i)
    {
        for (int j = 0; j < n - 1 - i; ++j)
        {
            if (dist[j + 1] > dist[j])
            {
                std::swap(dist[j], dist[j + 1]);
                std::swap(ids[j], ids[j + 1]);
            }
        }
    }

    std::vector<int> newSol(n);
    newSol[0] = ids[0];
    newSol[1] = ids[1];

    auto partialCost = [&](int size)->double{
        double sum = 0.0;
        for (int k = 0; k < size - 1; ++k)
            sum += edgeCost(pts[newSol[k]], pts[newSol[k+1]]);
        return sum;
    };

    for (int i = 2; i < n; ++i)
    {
        newSol[i] = ids[i];

        int bestPos = i;
        double bestLen = partialCost(i + 1);

        for (int j = i; j > 0; --j)
        {
            std::swap(newSol[j], newSol[j - 1]);
            const double testLen = partialCost(i + 1);
            if (testLen < bestLen)
            {
                bestLen = testLen;
                bestPos = j - 1;
            }
        }

        for (int j = 0; j < bestPos; ++j)
            std::swap(newSol[j], newSol[j + 1]);
    }

    m_order = std::move(newSol);
    evaluate();
}
