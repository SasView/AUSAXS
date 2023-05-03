#pragma once

#include <rigidbody/selection/BodySelectStrategy.h>
#include <settings/RigidBodySettings.h>

#include <memory>

namespace rigidbody {
    namespace factory {
        /**
         * @brief Prepare a body selection strategy.
         */
        std::unique_ptr<selection::BodySelectStrategy> create_selection_strategy(const RigidBody*, settings::rigidbody::BodySelectStrategyChoice choice = settings::rigidbody::body_select_strategy);
    }
}