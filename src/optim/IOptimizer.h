#pragma once

#include "../Tour.h"

class IOptimizer
{
public:
    virtual ~IOptimizer() = default;

    // Do a small step/iteration. Return true if the internal best solution improved.
    virtual bool iterate() = 0;

    virtual const Tour& bestTour() const = 0;
    virtual double baselineCost() const = 0;
};
