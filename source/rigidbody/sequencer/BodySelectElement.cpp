#include <rigidbody/sequencer/BodySelectElement.h>

using namespace rigidbody::sequencer;

BodySelectElement::BodySelectElement(LoopElement* owner) : LoopElementCallback(owner), strategy(settings::rigidbody::body_select_strategy) {}
BodySelectElement::BodySelectElement(LoopElement* owner, settings::rigidbody::BodySelectStrategyChoice strategy) : LoopElementCallback(owner), strategy(strategy) {}
BodySelectElement::~BodySelectElement() = default;

void BodySelectElement::apply() {
    std::cout << "BodySelectElement::apply()" << std::endl;
}