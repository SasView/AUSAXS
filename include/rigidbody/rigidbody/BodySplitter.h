#pragma once

#include <data/DataFwd.h>
#include <io/IOFwd.h>

#include <vector>

namespace rigidbody {
    struct BodySplitter {
        /**
         * @brief Load the structural data from a file and split it into multiple bodies at the designated indices.
         */
        static data::Molecule split(const io::File& input, std::vector<int> splits);

        /**
         * @brief Load the structural data from a file and split it into multiple bodies based on the chainID. 
         */
        static data::Molecule split(const io::File& input);
    };
}