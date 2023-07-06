#include <math/MovingAverager.h>

#include <utility/Exceptions.h>

void MovingAverage::validate_input(unsigned int N, unsigned int window_size) {
    if (N < window_size) {
        throw except::invalid_argument("MovingAverager::average: Window size is larger than data size.");
    }

    if (window_size % 2 == 0) {
        throw except::invalid_argument("Error in MovingAverager::validate_input: Window_size must be odd");
    }
}