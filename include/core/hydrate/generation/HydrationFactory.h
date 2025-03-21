#pragma once

#include <hydrate/generation/HydrationStrategy.h>
#include <utility/observer_ptr.h>
#include <grid/GridFwd.h>
#include <settings/MoleculeSettings.h>

#include <memory>

namespace ausaxs::hydrate {
    namespace factory {
        std::unique_ptr<HydrationStrategy> construct_hydration_generator(observer_ptr<data::Molecule> protein);
        std::unique_ptr<HydrationStrategy> construct_hydration_generator(observer_ptr<data::Molecule> protein, const settings::hydrate::HydrationStrategy& choice);
        std::unique_ptr<HydrationStrategy> construct_hydration_generator(observer_ptr<data::Molecule> protein, const settings::hydrate::HydrationStrategy& choice, const settings::hydrate::CullingStrategy& culling_strategy);
    }
}