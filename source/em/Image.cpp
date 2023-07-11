#include <em/Image.h>
#include <em/detail/header/MapHeader.h>
#include <settings/EMSettings.h>
#include <data/Atom.h>
#include <utility/Axis3D.h>

using namespace em;

Image::Image(em::detail::header::MapHeader* header, unsigned int layer) : N(header->get_axes().x.bins), M(header->get_axes().y.bins), header(header), data(N, M), z(layer), bounds(N, M) {}

Image::Image(const Matrix<float>& data) : N(data.N), M(data.M), data(data), bounds(N, M) {}

Image::Image(const Matrix<float>& data, em::detail::header::MapHeader* header, unsigned int layer) : N(data.N), M(data.M), header(header), data(data), z(layer), bounds(N, M) {}

void Image::set_z(unsigned int z) {this->z = z;}

unsigned int Image::get_z() const {return z;}

float Image::index(unsigned int x, unsigned int y) const {return data.index(x, y);}
float& Image::index(unsigned int x, unsigned int y) {return data.index(x, y);}

std::list<Atom> Image::generate_atoms(double cutoff) const {
    if (header == nullptr) [[unlikely]] {throw except::invalid_operation("Image::generate_atoms: Header must be initialized to use this method.");}
    std::list<Atom> atoms;
    auto map_axes = header->get_axes();

    // loop through all pixels in this image
    double xscale = map_axes.x.width();
    double yscale = map_axes.y.width();
    double zscale = map_axes.z.width();
    unsigned int step = settings::em::sample_frequency;
    
    // define a weight function for more efficient switching. 
    auto weight = settings::em::fixed_weights ? 
        [](float) {return 1.0f;} :      // fixed weights enabled - all voxels have the same weight of 1
        [] (float val) {return val;};   // fixed weights disabled - voxels have a weight equal to their density
    
    for (unsigned int x = 0; x < N; x += step) {
        for (unsigned int y = bounds[x].min; y < bounds[x].max; y += step) {
            float val = index(x, y);
            if (val < cutoff) {
                continue;
            }
            atoms.push_back(Atom(0, "C", "", "LYS", "", 0, "", {x*xscale, y*yscale, z*zscale}, weight(val), 0, "C", ""));
        }
    }
    return atoms;
}

unsigned int Image::count_voxels(double cutoff) const {
    unsigned int count = 0;
    unsigned int step = settings::em::sample_frequency;
    for (unsigned int x = 0; x < N; x += step) {
        for (unsigned int y = bounds[x].min; y < bounds[x].max; y += step) {
            if (index(x, y) >= cutoff) {
                count++;
            }
        }
    }
    return count;
}

double Image::squared_sum() const {
    double sum = 0;
    for (unsigned int x = 0; x < N; x++) {
        for (unsigned int y = 0; y < M; y++) {
            sum += std::pow(index(x, y), 2);
        }
    }
    return sum;
}

hist::Histogram2D Image::as_hist() const {
    if (header == nullptr) [[unlikely]] {throw except::invalid_operation("Image::as_hist: Header must be initialized to use this method.");}
    auto map_axes = header->get_axes();
    hist::Histogram2D hist(map_axes.x, map_axes.y);

    for (unsigned int x = 0; x < N; x++) {
        for (unsigned int y = 0; y < M; y++) {
            hist.index(x, y) = index(x, y);
        }
    }
    return hist;
}

double Image::mean() const {
    double sum = 0;
    for (unsigned int x = 0; x < N; x++) {
        for (unsigned int y = 0; y < M; y++) {
            sum += index(x, y);
        }
    }

    return sum/(N*M);
}

Limit Image::limits() const {
    double min = 1e9, max = -1e9;
    for (unsigned int x = 0; x < N; x++) {
        for (unsigned int y = 0; y < M; y++) {
            double val = index(x, y);
            min = std::min(min, val);
            max = std::max(max, val);
        }
    }

    return Limit(min, max);
}

void Image::set_header(em::detail::header::MapHeader* header) {
    this->header = header;
}

const ObjectBounds2D& Image::get_bounds() const {
    return bounds;
}

const ObjectBounds2D& Image::setup_bounds(double cutoff) {
    for (unsigned int x = 0; x < N; x++) {
        bounds.set_bounds(x, 0, 0);
        bool min_set = false;
        for (unsigned int y = 0; y < M; y++) {
            if (index(x, y) < cutoff) {continue;}
            if (!min_set) {
                bounds.set_bounds(x, y, y); // update min val to this index, and also set max in case this is the only entry
                min_set = true;
            } else {
                bounds.set_max(x, y);
            }
        }
    }

    return bounds;
}

bool Image::operator==(const Image& other) const {
    return data == other.data && N == other.N && M == other.M && z == other.z;
}

std::string Image::to_string() const {return data.to_string();}