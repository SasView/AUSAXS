#pragma once

#include <Symbols.h>
#include <utility/Concepts.h>

#include <vector>
#include <signal.h>
#include <concepts>

template<numeric T> class Vector;

/**
 * @brief Slice
 * 
 * A Slice is a type of cut of a Matrix, typically either in the form of a single row or column. 
 * The idea with this class is to give mutable access to such a structure. 
 */
template<numeric T>
class Slice {
    public:
        /**
         * @brief Constructor. 
         * 
         * @param N The number of rows of this Slice. 
         * @param M The number of columns of this Slice. 
         * @param start The start location in the raw data array. 
         * @param step The number of elements to skip between each index in the raw data array. 
         * @param length The total number of elements this Slice can access. 
         */
        Slice(unsigned int N, unsigned int M, unsigned int start, unsigned int step, unsigned int length) : N(N), M(M), start(start), step(step), length(length) {}

        /**
         * @brief Get the dot product with another Slice. 
		 * 		  Complexity: O(N)
         */
        template<typename Q>
        double dot(const Slice<Q>& s) {return dot(s.operator Vector<T>());}

        /**
         * @brief Get the dot product with a Vector.
		 * 		  Complexity: O(N)
         */
        template<typename Q>
        double dot(const Vector<Q>& v) {return operator Vector<T>().dot(v);}

        /**
         * @brief Get the norm of this Slice. 
		 * 		  Complexity: O(N)
		 */
        double norm() {return operator Vector<T>().norm();}

		/**
		 * @brief Get the length of this Slice.
		 */
		unsigned int size() const noexcept {return length;}

        template<typename Q>
        bool operator==(const Slice<Q>& rhs) const {
            #if SAFE_MATH
                if (size() != rhs.size()) {
                    throw std::invalid_argument("ConstSlice::operator==: Slice of size \"" + std::to_string(rhs.size()) + "\" does not fit in slice of size \"" + std::to_string(size()) + "\".");
                }
            #endif

            bool equal = true;
            for (unsigned int i = 0; i < size(); i++) {
                equal = equal && (*this)[i] == rhs[i];
            }
            return equal;
        }

        template<typename Q>
        bool operator!=(const Slice<Q>& rhs) const {
            return !((*this) == rhs);
        }

        /**
         * @brief Cast this Slice into a Vector. 
		 * 		  Complexity: O(N)
         */
        operator Vector<T>() const {
            Vector<T> v(length);
            for (unsigned int i = 0; i < length; i++) {
                v[i] = (*this)[i];
            }
            return v;
        }

		/**
		 * @brief Cast this Slice into a std::vector.
		 * 		  Complexity: O(N)
		 */
		operator std::vector<T>() const {
			std::vector<T> v(length);
			for (unsigned int i = 0; i < length; i++) {
				v[i] = (*this)[i];
			}
			return v;
		}

		/**
		 * @brief Cast this Slice into a std::vector.
		 * 		  Complexity: O(N)
		 */
		std::vector<T> to_vector() const {
			return operator std::vector<T>();
		}

        /**
         * @brief Mutable indexer in this Slice.
		 * 		  Complexity: O(1)
		 */
        virtual const T& operator[](unsigned int j) const = 0;

		/**
		 * @brief Get the final element in this Slice.
		 */
		const T& back() const {
			#if SAFE_MATH
				if (size() == 0) {throw std::out_of_range("Slice::back(): Slice is empty.");}
			#endif
			return (*this)[length-1];
		}

		/**
		 * @brief Get the final element in this Slice.
		 */
		const T& first() const {
			#if SAFE_MATH
				if (size() == 0) {throw std::out_of_range("Slice::first(): Slice is empty.");}
			#endif
			return (*this)[0];
		}
    
        /**
         * @brief Get a string representation of this Slice. 
         */
        std::string to_string() const {return operator Vector<T>().to_string();}

        /**
         * @brief Output the string representation of this vector to a stream. 
         */
        friend std::ostream& operator<<(std::ostream& os, const Slice<T>& s) {os << s.to_string(); return os;}

        const unsigned int N, M;
        unsigned int start, step, length;
};