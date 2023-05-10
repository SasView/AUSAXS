#include <rigidbody/transform/TransformStrategy.h>
#include <rigidbody/RigidBody.h>

#include <vector>

using namespace rigidbody;

struct BackupBody {
    BackupBody(const Body& body, unsigned int index) : body(body), index(index) {}
    Body body;
    unsigned int index;
};

struct TransformGroup {
    TransformGroup(std::vector<Body*> bodies, std::vector<unsigned int> indices, std::shared_ptr<DistanceConstraint> target, Vector3<double> pivot);
    std::vector<Body*> bodies;                  // The bodies to transform.
    std::vector<unsigned int> indices;          // The indices of the bodies in the rigidbody.
    std::shared_ptr<DistanceConstraint> target; // The constraint to transform along.
    Vector3<double> pivot;                      // The pivot point of the transformation.
};

TransformGroup::TransformGroup(std::vector<Body*> bodies, std::vector<unsigned int> indices, std::shared_ptr<DistanceConstraint> target, Vector3<double> pivot) 
    : bodies(bodies), indices(indices), target(target), pivot(pivot) {}

void TransformStrategy::rotate(const Matrix<double>& M, TransformGroup& group) {
    std::for_each(group.bodies.begin(), group.bodies.end(), [&group] (Body* body) {body->translate(-group.pivot);});
    std::for_each(group.bodies.begin(), group.bodies.end(), [&M]     (Body* body) {body->rotate(M);});
    std::for_each(group.bodies.begin(), group.bodies.end(), [&group] (Body* body) {body->translate(group.pivot);});
}

void TransformStrategy::translate(const Vector3<double>& t, TransformGroup& group) {
    std::for_each(group.bodies.begin(), group.bodies.end(), [&t] (Body* body) {body->translate(t);});
}

void TransformStrategy::undo() {
    for (auto& body : bodybackup) {
        rigidbody->bodies[body.index] = std::move(body.body);
    }
    bodybackup.clear();
}

void TransformStrategy::backup(TransformGroup& group) {
    bodybackup.clear();
    for (unsigned int i = 0; i < group.bodies.size(); i++) {
        bodybackup.emplace_back(*group.bodies[i], group.indices[i]);
    }
}
