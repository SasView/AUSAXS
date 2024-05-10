#pragma once

#include <hydrate/generation/GridBasedHydration.h>
#include <math/MathFwd.h>

namespace hydrate {
    /**
     * @brief This strategy iterates through all atoms and attempts to place a water molecule at x±r, y±r, and z±r. 
     * If the location is valid, the molecule will be placed. This will typically generate a lot of molecules, 
     * and so a culling method may be useful afterwards. 
     * 
     * The radius r is defined as the sum of @a ra and @a rh.
     */
    class AxesHydration : public GridBasedHydration {
        public:
            AxesHydration(observer_ptr<data::Molecule> protein);
            virtual ~AxesHydration();

            std::vector<grid::GridMember<data::record::Water>> generate_explicit_hydration() override;

        private:
            observer_ptr<grid::Grid> grid;

            void initialize() override;

            /**
            * @brief Check if a water molecule can be placed at the given location. 
            * @param loc the location to be checked. 
            * @return True if this is an acceptable location, false otherwise.
            */
            bool collision_check(const Vector3<unsigned int>& loc, double ra) const;
    };
}