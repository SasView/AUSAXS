#pragma once

#include "data/Protein.h"
#include "rigidbody/TransformationStrategy.h"

/**
 * @brief \class RigidTransform. 
 * 
 * With this transformation strategy, everything connected to the target of the transformation will be transformed as well. 
 */
class RigidTransform : public TransformationStrategy {
  public:
    /**
     * @brief Construtor. 
     */
    RigidTransform(const Protein& protein) : TransformationStrategy(protein) {}

    /**
     * @brief Destructor.
     */
    ~RigidTransform() override = default;

    /**
     * @brief Rotate a body. 
     */
    void rotate(const Vector3& axis, const double rad, Body& body) override {
        body.rotate(axis, rad);
    }

    /**
     * @brief Translate a body. 
     */
    void translate(const Vector3& v, Body& body) override {}
};