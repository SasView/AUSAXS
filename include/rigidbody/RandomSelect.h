#pragma once

#include <random>

#include "rigidbody/BodySelectStrategy.h"
#include "data/Protein.h"

/**
 * @brief \class RandomSelect
 * 
 * This selection strategy randomly selects a Body. 
 */
class RandomSelect : public BodySelectStrategy {
  public: 
    /**
     * @brief Constructor.
     */
    RandomSelect(const Protein& protein) : BodySelectStrategy(protein) {
        std::random_device random;
        generator = std::mt19937(random());
        distribution = std::uniform_int_distribution<int>(0, protein.bodies.size());
    }

    /**
     * @brief Destructor.
     */
    ~RandomSelect() override = default;

    /**
     * @brief Get the index of the next body to be transformed. 
     */
    size_t next() override {
        return distribution(generator);
    }

  private:
    std::mt19937 generator;                          // The random number generator. 
    std::uniform_int_distribution<int> distribution; // The random number distribution. 
};