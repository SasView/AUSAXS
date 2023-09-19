#pragma once

namespace hist::detail {
    // The form factor type of an atom. This is intended to be used as an index for best performance.
    enum class form_factor_t {
        NEUTRAL_HYDROGEN,
        NEUTRAL_CARBON,
        NEUTRAL_NITROGEN,
        NEUTRAL_OXYGEN,
        OTHER,
        COUNT
    };
}