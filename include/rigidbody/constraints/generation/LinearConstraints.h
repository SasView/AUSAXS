#pragma once

#include <rigidbody/constraints/generation/ConstraintGenerationStrategy.h>

namespace rigidbody {
    class LinearConstraints : public ConstraintGenerationStrategy {
        public:
            using ConstraintGenerationStrategy::ConstraintGenerationStrategy;

            /**
             * @brief Generate a constraint.
             */
            std::vector<DistanceConstraint> generate() const override;
    };
}