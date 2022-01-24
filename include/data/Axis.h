#pragma once

#include <vector>
#include <array>
#include <initializer_list>
#include <math.h>

struct Limit {
    Limit(double min, double max) : min(min), max(max) {}

    double span() const {return max-min;}

    double min, max;
};

class Limit3D {
  public:
    Limit3D(const Limit& x, const Limit& y, const Limit& z) : x(x), y(y), z(z) {}
    Limit3D(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax) : x(xmin, xmax), y(ymin, ymax), z(zmin, zmax) {}

    Limit x, y, z;  
};

class Axis {
  public:
    Axis() : bins(0), min(0), max(0) {}
    Axis(const Limit& limits, int bins) : bins(bins), min(limits.min), max(limits.max) {}
    Axis(const int bins, const double xmin, const double xmax) : bins(bins), min(xmin), max(xmax)  {}

    Axis& operator=(std::initializer_list<double> list) {
        std::vector<double> d = list;
        bins = std::round(d[0]); 
        min = d[1];
        max = d[2];
        return *this;
    }

    std::string to_string() const {
        return "Axis: (" + std::to_string(min) + ", " + std::to_string(max) + ") with " + std::to_string(bins) + " bins";
    }

    friend std::ostream& operator<<(std::ostream& os, const Axis& axis) {os << axis.to_string(); return os;}

    double width() const {
        return (max-min)/bins;
    }

    int bins;
    double min, max;
};

class Axis3D {
  public:
    Axis3D() {}
    Axis3D(const Axis& x, const Axis& y, const Axis& z) : x(x), y(y), z(z) {}
    Axis3D(const Limit3D& limits, double width) : x(limits.x, limits.x.span()/width), y(limits.y, limits.y.span()/width), z(limits.z, limits.z.span()/width) {}
    Axis3D(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax, int bins) : x(bins, xmin, xmax), y(bins, ymin, ymax), z(bins, zmin, zmax) {}
    Axis3D(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax, double width) : x((xmax-xmin)/width, xmin, xmax), y((ymax-ymin)/width, ymin, ymax), z((zmax-zmin)/width, zmin, zmax) {}

    Axis x, y, z;
};