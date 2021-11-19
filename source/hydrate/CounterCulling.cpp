#include "hydrate/CullingStrategy.h"
#include "hydrate/Grid.h"

#include "TVector3.h"

/**
 * @brief Iterates through all placed water molecules, rejecting all but the nth, where n is determined from the desired number of water molecules. 
 */
class CounterCulling : public CullingStrategy {
public:
    using CullingStrategy::CullingStrategy;

    // runs in O(n) where n is the number of water molecules
    vector<shared_ptr<Hetatom>> cull(vector<shared_ptr<Hetatom>> placed_water) const override {
        if (target_count == 0) {
            return placed_water;
        }

        int factor = std::floor(placed_water.size()/target_count); // reduction factor
        if (factor < 2) {
            return placed_water;
        }

        vector<shared_ptr<Hetatom>> final_water(placed_water.size(), nullptr); // the final water molecules that will be used
        int c = 0; // counter
        int i = 0; // index
        for (const auto& a : placed_water) {
            c++;
            if (c % factor != 0) {
                grid->remove(a);
                continue;
            }
            final_water[i] = a;
            i++;
        }
        final_water.resize(i);
        return final_water;
    }
};