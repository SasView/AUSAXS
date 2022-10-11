#pragma once

#include <math/Vector3.h>

template<typename T>
T Vector3<T>::operator[] (unsigned int i) const {
    #if (SAFE_MATH) 
        if (i >= 3) {
            throw std::invalid_argument("Error in Vector3::operator[]: Index out of bounds (" + std::to_string(i) + " >= 3)");
        }
    #endif
    return data[i];
}

template<typename T>
T& Vector3<T>::operator[] (unsigned int i) {
    #if (SAFE_MATH) 
        if (i >= 3) {
            throw std::invalid_argument("Error in Vector3::operator[]: Index out of bounds (" + std::to_string(i) + " >= 3)");
        }
    #endif
    return data[i];
}

template<typename T>
Vector3<T>& Vector3<T>::operator=(const Vector3<T>& v) {
    data = v.data;
    return *this;
}

template<typename T>
Vector3<T>& Vector3<T>::operator=(std::initializer_list<T> l) {
    if (l.size() != 3) {
        throw std::invalid_argument("Vector3: Initializer list must have size 3");
    }
    data[0] = *l.begin();
    data[1] = *(l.begin() + 1);
    data[2] = *(l.begin() + 2);
    return *this;
}

template<typename T> template<typename Q>
Vector3<T>& Vector3<T>::operator+=(const Vector3<Q>& v) {
    x() += v.x();
    y() += v.y();
    z() += v.z();
    return *this;
}

template<typename T> template<typename Q>
Vector3<T>& Vector3<T>::operator-=(const Vector3<Q>& v) {
    x() -= v.x();
    y() -= v.y();
    z() -= v.z();
    return *this;
}

template<typename T>
Vector3<T>& Vector3<T>::operator/=(double a) {
    x() /= a;
    y() /= a;
    z() /= a;
    return *this;
}

template<typename T>
Vector3<T>& Vector3<T>::operator*=(double a) {
    x() *= a;
    y() *= a;
    z() *= a;
    return *this;
}

template<typename T> template<typename Q>
Vector3<T>& Vector3<T>::operator*=(const Vector3<Q>& v) {
    x() *= v.x();
    y() *= v.y();
    z() *= v.z();
    return *this;
}

template<typename T> template<typename Q>
bool Vector3<T>::operator==(const Vector3<Q>& v) const {
	return abs(double(x()-v.x())) + abs(double(y()-v.y())) + abs(double(z()-v.z())) < precision;
}

template<typename T> template<typename Q>
bool Vector3<T>::equals(const Vector3<Q>& v, double p) const {
	return abs(double(x()-v.x())) + abs(double(y()-v.y())) + abs(double(z()-v.z())) < p;
}
template<typename T> template<typename Q>
bool Vector3<T>::operator!=(const Vector3<Q>& v) const {
	return !(*this == v);
}

template<typename T> template<typename Q>
double Vector3<T>::dot(const Vector3<Q>& v) const {
    return x() * v.x() + y() * v.y() + z() * v.z();
}

template<typename T>
double Vector3<T>::norm() const {return sqrt(dot(*this));}

template<typename T>
double Vector3<T>::magnitude() const {return norm();}

template<typename T> template<typename Q>
double Vector3<T>::distance(const Vector3<Q>& v) const {return sqrt(distance2(v));}

template<typename T> template<typename Q>
double Vector3<T>::distance2(const Vector3<Q>& v) const {return pow(x()-v.x(), 2) + pow(y()-v.y(), 2) + pow(z()-v.z(), 2);}

template<typename T> template<typename Q>
Vector3<T> Vector3<T>::cross(const Vector3<Q>& v) const {return {y()*v.z() - v.y()*z(), z()*v.x() - v.z()*x(), x()*v.y() - v.x()*y()};}

template<typename T> template<typename Q>
void Vector3<T>::rotate(const Vector3<Q>& axis, double angle) {
    Matrix R = matrix::rotation_matrix(axis, angle);
    rotate(R);
}

template<typename T>
void Vector3<T>::rotate(const Matrix<double>& matrix) {
    *this = matrix*(*this);
}

template<typename T>
Vector3<double>& Vector3<T>::normalize() {
    *this /= this->norm();
    return *this;
}

template<typename T>
std::tuple<Vector3<double>, Vector3<double>, Vector3<double>> Vector3<T>::generate_basis() {
    return vector3::generate_basis(*this);
}

template<typename T>
std::string Vector3<T>::to_string(std::string message) const {
    return message + "(" + std::to_string(x()) + ", " + std::to_string(y()) + ", " + std::to_string(z()) + ")";
}

template<typename T>
Vector3<T>::operator std::vector<T>() {
    return std::vector<T>(data);
}

template<typename T>
Vector3<T>::operator Vector<T>() {
    return Vector<T>({x(), y(), z()});
}

template<typename T>
Vector3<T>::operator Matrix<T>() {
    return Matrix<T>({{x()}, {y()}, {z()}});
}

template<typename T>
size_t Vector3<T>::size() const {return 3;}

template<typename T>
Vector3<T> Vector3<T>::copy() const {
    return Vector3<T>(x(), y(), z());
}

template<typename T> typename std::array<T, 3>::iterator Vector3<T>::begin() {return data.begin();}
template<typename T> typename std::array<T, 3>::iterator Vector3<T>::end() {return data.end();}
template<typename T> const typename std::array<T, 3>::const_iterator Vector3<T>::begin() const {return data.begin();}
template<typename T> const typename std::array<T, 3>::const_iterator Vector3<T>::end() const {return data.end();}

template<typename T> T& Vector3<T>::x() {return data[0];}
template<typename T> const T& Vector3<T>::x() const {return data[0];}

template<typename T> T& Vector3<T>::y() {return data[1];}
template<typename T> const T& Vector3<T>::y() const {return data[1];}

template<typename T> T& Vector3<T>::z() {return data[2];}
template<typename T> const T& Vector3<T>::z() const {return data[2];}