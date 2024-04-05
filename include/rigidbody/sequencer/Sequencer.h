#pragma once

#include <rigidbody/sequencer/LoopElement.h>
#include <rigidbody/RigidbodyFwd.h>
#include <data/DataFwd.h>
#include <io/IOFwd.h>
#include <utility/observer_ptr.h>

namespace rigidbody {
    namespace sequencer {
        class Sequencer : public LoopElement {
            public:
                Sequencer(const io::ExistingFile& saxs, observer_ptr<RigidBody> rigidbody);
                ~Sequencer();

                std::shared_ptr<fitter::Fit> execute() override;

                observer_ptr<RigidBody> _get_rigidbody() const override;

                observer_ptr<detail::BestConf> _get_best_conf() const override;

                observer_ptr<const Sequencer> _get_sequencer() const override;

    			bool _optimize_step() const;

            private:
                observer_ptr<RigidBody> rigidbody;
                std::unique_ptr<detail::BestConf> best;
        };
    }
}