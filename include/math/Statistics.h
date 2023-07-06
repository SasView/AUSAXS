#pragma once

#include <math/slices/Slice.h>

#include <vector>
#include <algorithm>
#include <numeric>
#include <type_traits>
#include <initializer_list>
#include <math.h>
#include <iostream>
#include <concepts>

namespace stats {
    namespace detail {
        template<typename C>
        concept ContainerType = requires(C c) {
            {c.size()} -> std::convertible_to<unsigned int>;
            {c[0]} -> std::convertible_to<double>;
        };

        /**
         * @brief Calculate the weighted mean of a container class.
         */
        template<ContainerType T, ContainerType Q>
        double weighted_mean(T& x, Q& xerr) noexcept {
            double sum_wx = 0;
            double sum_w = 0;
            for (unsigned int i = 0; i < x.size(); i++) {
                double w = 1.0/(xerr[i]*xerr[i]);
                sum_wx += w*x[i];
                sum_w += w;
            }
            return sum_wx/sum_w;
        }

        /**
         * @brief Calculate the error in the weighted mean of a container class.
         */
        template<ContainerType T>
        double weighted_mean_error(T& xerr) noexcept {
            double sum = 0;
            for (unsigned int i = 0; i < xerr.size(); i++) {
                double w = 1.0/(xerr[i]*xerr[i]);
                sum += w;
            }
            return std::sqrt(1.0/(sum));
        }

        /**
         * @brief Calculate the mean of a container class.
         */
        template<ContainerType T>
        double mean(T& v) noexcept {
            double sum = 0;
            for (unsigned int i = 0; i < v.size(); i++) {
                sum += v[i];
            }
            return sum/v.size();
        }

        /**
         * @brief Calculate the variance of a container class.
         */
        template<ContainerType T>
        double var(T& v, unsigned int ddof) noexcept {
            double mu = mean(v);
            double sum = 0;
            for (unsigned int i = 0; i < v.size(); i++) {
                sum += std::pow(v[i] - mu, 2);
            }
            return sum/(v.size() - ddof);
        }
    }

    /**
     * @brief Calculate the mode of a vector.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    T mode(const std::vector<T>& v) {
        if (v.empty()) {
            throw std::invalid_argument("stats::mode: Vector is empty.");
        }
        std::vector<T> v_copy = v;
        std::sort(v_copy.begin(), v_copy.end());
        T mode = v_copy[0];
        unsigned int max_count = 0;
        unsigned int count = 1;
        for (unsigned int i = 1; i < v_copy.size(); i++) {
            if (v_copy[i] == v_copy[i - 1]) {
                count++;
            } else {
                if (max_count < count) {
                    max_count = count;
                    mode = v_copy[i-1];
                }
            }
        }

        // handle edge-case where last sorted element is the mode
        if (max_count < count) {
            max_count = count;
            mode = v_copy.back();
        }
        return mode;
    }

    /**
     * @brief Calculate the mean of a vector.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double weighted_mean_error(std::vector<T> xerr) noexcept {
        return detail::weighted_mean_error(xerr);
    }

    /**
     * @brief Calculate the mean of a vector.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double weighted_mean_error(const ConstSlice<T>& xerr) noexcept {
        return detail::weighted_mean_error(xerr);
    }

    /**
     * @brief Calculate the weighted mean of a vector.
     */
    template<typename T, typename Q, typename = std::enable_if_t<std::is_arithmetic_v<T> && std::is_arithmetic_v<Q>>>
    double weighted_mean(const std::vector<T>& x, const std::vector<Q>& xerr) noexcept {
        return detail::weighted_mean(x, xerr);
    }

    /**
     * @brief Calculate the weighted mean of a Slice.
     */
    template<typename T, typename Q, typename = std::enable_if_t<std::is_arithmetic_v<T> && std::is_arithmetic_v<Q>>>
    double weighted_mean(const ConstSlice<T>& x, const ConstSlice<Q>& xerr) noexcept {
        return detail::weighted_mean(x, xerr);
    }

    /**
     * @brief Calculate the mean of a vector.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double mean(const std::vector<T>& v) noexcept {
        return detail::mean(v);
    }

    /**
     * @brief Calculate the mean of a Slice.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double mean(const ConstSlice<T>& s) noexcept {
        return detail::mean(s);
    }

    /**
     * @brief Calculate the mean of a list.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double mean(const std::initializer_list<T>& v) noexcept {
        return mean(std::vector<T>(v));
    }

    /**
     * @brief Calculate the variance of a Slice.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double var(const ConstSlice<T>& s, unsigned int ddof = 1) noexcept {
        return detail::var(s, ddof);
    }

    /**
     * @brief Calculate the variance of a vector.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double var(const std::vector<T>& v, unsigned int ddof = 1) noexcept {
        return detail::var(v, ddof);
    }

    /**
     * @brief Calculate the variance of a list.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double var(const std::initializer_list<T>& v, unsigned int ddof = 1) noexcept {
        return var(std::vector<T>(v), ddof);
    }

    /**
     * @brief Calculate the standard deviation of a Slice.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double std(const ConstSlice<T>& s, unsigned int ddof = 1) noexcept {
        return std::sqrt(var(s, ddof));
    }

    /**
     * @brief Calculate the standard deviation of a vector.
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double std(const std::vector<T>& v, unsigned int ddof = 1) noexcept {
        return std::sqrt(var(v, ddof));
    }

    /**
     * @brief Combine a list of errors to a single error of the mean. 
     */
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    double std(const std::initializer_list<T>& v, unsigned int ddof = 1) noexcept {
        return std(std::vector<T>(v), ddof);
    }

    template<typename T>
    struct Measurement {
        Measurement() {}
        Measurement(const std::vector<T>& vals) : vals(vals) {}

        double mean() const noexcept {return mean(vals);}
        double std() const noexcept {return std(vals);}
        double var() const noexcept {return var(vals);}

        std::vector<T> vals;
    };
}