#pragma once

#include <string>

/**
 * @brief \class Writer. 
 *               Virtual super-class for all data file writers. 
 */
class Writer {
    public:
        virtual ~Writer() = default;

        /**
         * @brief Write the contents of the backing File to a given path. 
         */
        virtual void write(std::string) = 0;
};