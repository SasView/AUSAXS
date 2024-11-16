#pragma once

#include <rigidbody/selection/BodySelectStrategy.h>

namespace ausaxs::rigidbody {
	namespace selection {
		/**
		 * @brief Sequential body selection strategy.
		 * 
		 * This selection strategy selects the bodies sequentially, excluding all the constraints of each body.
		 * Example: If we have the bodies A(3), B(2), C(2) where A has 3 constraints, B has 2 constraints and C has 2 constraints, the order of selection would be A(random), B(random), C(random).
		 */
		class SequentialBodySelect : public BodySelectStrategy {
			public: 
				SequentialBodySelect(const RigidBody* rigidbody);
				~SequentialBodySelect() override;

				std::pair<unsigned int, int> next() override; ///< @copydoc BodySelectStrategy::next()

			private:
				unsigned int ibody = 0; 		// The index of the body to be transformed. 
				unsigned int iconstraint = 0; 	// The index of the constraint to be transformed.
		};	
	}

}