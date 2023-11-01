#pragma once

#include <hist/distribution/Distribution2D.h>
#include <hist/distribution/WeightedDistribution2D.h>

namespace hist {
    template <bool UseWeightedContainer>
    struct GenericDistribution2D;

    template <>
    struct GenericDistribution2D<true> {
        using type = WeightedDistribution2D;
    };

    template <>
    struct GenericDistribution2D<false> {
        using type = Distribution2D;
    };
}