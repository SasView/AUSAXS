#include <crystal/miller/ReducedMillers.h>

using namespace crystal;

ReducedMillers::ReducedMillers() : h(0), k(0), l(0) {}

ReducedMillers::ReducedMillers(unsigned int h, unsigned int k, unsigned int l) : h(h), k(k), l(l) {}

std::vector<Miller> ReducedMillers::generate() const {
    std::vector<Miller> bases = generate_independent_bases();
    std::vector<Miller> millers;

    // now generate all millers indices
    // we can do this by multiplying the base pairs with integers
    for (const auto& base : bases) {
        int multiplier = 0;
        while (multiplier++ < 100000) { // hard limit to prevent infinite loop
            if (base.h*multiplier > h || base.k*multiplier > k || base.l*multiplier > l) {break;}
            millers.emplace_back(base.h*multiplier, base.k*multiplier, base.l*multiplier);
        }
    }

    return millers;
}

std::vector<Miller> ReducedMillers::generate_independent_bases() const {
    double limit = 4;
    double limit2 = limit*limit;

    std::vector<Miller> millers;
    for (int h = -limit; h <= limit; h++) {
        for (int k = -limit; k <= limit; k++) {
            for (int l = -limit; l <= limit; l++) {
                if (h*h + k*k + l*l <= limit2) {
                    millers.emplace_back(h, k, l);
                }
            }
        }
    }

    // filter out Friedel symmetry equivalent pairs
    std::vector<Miller> friedel_independent;
    for (int i = 0; i < millers.size(); i++) {
        bool is_independent = true;
        for (int j = 0; j < i; j++) {
            if (millers[i].friedel_equivalent(millers[j])) {
                is_independent = false;
                break;
            }
        }
        if (is_independent) {friedel_independent.push_back(millers[i]);}
    }

    // sort the millers indices by their length
    std::sort(friedel_independent.begin(), friedel_independent.end(), [](const Miller& a, const Miller& b) {
        return a.length2() < b.length2();
    });

    // since the indices are sorted, we only check the previous indices for linear independence
    // thus we will keep the shortest bases and discard the longer ones
    std::vector<Miller> linearly_independent;
    for (int i = 1; i < friedel_independent.size(); i++) {
        bool is_independent = true;
        for (int j = 0; j < i; j++) {
            for (int k = 1; k <= limit; k++) {
                if (friedel_independent[i] == friedel_independent[j]*k) {
                    is_independent = false;
                    break;
                }
                if (!is_independent) {break;}
            }
        }
        if (is_independent) {linearly_independent.push_back(friedel_independent[i]);}
    }

    return linearly_independent;
}