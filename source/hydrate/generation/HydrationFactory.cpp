/*
This software is distributed under the GNU Lesser General Public License v3.0. 
For more information, please refer to the LICENSE file in the project root.
*/

#include <hydrate/generation/HydrationFactory.h>
#include <hydrate/generation/JanHydration.h>
#include <hydrate/generation/RadialHydration.h>
#include <hydrate/generation/AxesHydration.h>
#include <hydrate/generation/PepsiHydration.h>
#include <hydrate/generation/NoHydration.h>
#include <settings/MoleculeSettings.h>
#include <utility/Exceptions.h>
#include <math/Vector3.h>

using namespace hydrate;

std::unique_ptr<HydrationStrategy> hydrate::factory::construct_hydration_generator(observer_ptr<data::Molecule> protein) {
    return hydrate::factory::construct_hydration_generator(protein, settings::hydrate::hydration_strategy);
}

std::unique_ptr<HydrationStrategy> hydrate::factory::construct_hydration_generator(observer_ptr<data::Molecule> protein, const settings::hydrate::HydrationStrategy& choice) {
    switch (choice) {
        case settings::hydrate::HydrationStrategy::AxesStrategy: 
            return std::make_unique<AxesHydration>(protein);
        case settings::hydrate::HydrationStrategy::RadialStrategy:
            return std::make_unique<RadialHydration>(protein);
        case settings::hydrate::HydrationStrategy::JanStrategy: 
            return std::make_unique<JanHydration>(protein);
        case settings::hydrate::HydrationStrategy::NoStrategy: 
            return std::make_unique<NoHydration>();
        case settings::hydrate::HydrationStrategy::PepsiStrategy:
            return std::make_unique<PepsiHydration>(protein);
        default: 
            throw except::unknown_argument("hydrate::factory::construct_hydration_generator: Unkown HydrationStrategy. Did you forget to add it to the switch statement?");
    }
}
