#include <table/ArrayDebyeTable.h>

using namespace table;

#if DEBUG 
    #include <iostream>
    #include <utility/Console.h>
    #include <utility/Utility.h>
    #include <settings/HistogramSettings.h>
#endif
void ArrayDebyeTable::check_default(const std::vector<double>& q, const std::vector<double>& d) {
    #if DEBUG 
        const Axis& axis = constants::axes::q_axis;

        if (q.size() != axis.bins) [[unlikely]] {
            console::print_warning("Warning in DebyeLookupTable::initialize: Not using default tables.");
            std::cout << "\tReason: q.size() != axis.bins" << std::endl;
        }

        if (q[0] != axis.min) [[unlikely]] {
            console::print_warning("Warning in DebyeLookupTable::initialize: Not using default tables.");
            std::cout << "\tReason: q[0] != axis.min" << std::endl;
        }

        if (q[1] != axis.min + (axis.max-axis.min)/axis.bins) [[unlikely]] {
            console::print_warning("Warning in DebyeLookupTable::initialize: Not using default tables.");
            std::cout << "\tReason: q[1] != axis.min + (axis.max-axis.min)/axis.bins" << std::endl;
        }

        if (q[2] != axis.min + 2*(axis.max-axis.min)/axis.bins) [[unlikely]] {
            console::print_warning("Warning in DebyeLookupTable::initialize: Not using default tables.");
            std::cout << "\tReason: q[2] != axis.min + 2*(axis.max-axis.min)/axis.bins" << std::endl;
        }

        // check empty
        if (d.empty()) [[unlikely]] {
            console::print_warning("Warning in DebyeLookupTable::initialize: Not using default tables.");
            std::cout << "\tReason: d.empty()" << std::endl;
        }

        // check if too large for default table
        if (d.back() > constants::axes::d_axis.max) [[unlikely]] {
            console::print_warning("Warning in DebyeLookupTable::initialize: Not using default tables.");
            std::cout << "\tReason: d.back() > default_size" << std::endl;
        }
        
        // check first width (d[1]-d[0] may be different from the default width)
        if (!utility::approx(d[2]-d[1], constants::axes::d_axis.width())) [[unlikely]] {
            console::print_warning("Warning in DebyeLookupTable::initialize: Not using default tables.");
            std::cout << "\tReason: !utility::approx(d[2]-d[1], width)" << std::endl;
        }
        
        // check second width
        if (!utility::approx(d[3]-d[2], constants::axes::d_axis.width())) [[unlikely]] {
            console::print_warning("Warning in DebyeLookupTable::initialize: Not using default tables.");
            std::cout << "\tReason: !utility::approx(d[3]-d[2], width)" << std::endl;
        }
    #endif
}

inline constexpr ArrayDebyeTable default_table;
const ArrayDebyeTable& ArrayDebyeTable::get_default_table() {
    return default_table;
}