#include <vector>
#include <cmath>
#include <fstream>
#include <random>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <TGraph.h>
#include <TGraphErrors.h>

#include <utility/Utility.h>
#include <utility/Dataset.h>
#include <utility/Exceptions.h>
#include <utility/Settings.h>
#include <math/SimpleLeastSquares.h>

using std::vector, std::string;

Dataset::Dataset(const string file) {
    read(file);
}

Dataset::Dataset(std::string xlabel, std::string ylabel) noexcept: xlabel(xlabel), xerrlabel(xlabel+"err"), ylabel(ylabel), yerrlabel(ylabel+"err") {
    plot_options.xlabel = xlabel;
    plot_options.ylabel = ylabel;
}

Dataset::Dataset(const std::vector<double>& x, const std::vector<double>& y) noexcept: x(x), y(y) {
    validate_sizes();
}

Dataset::Dataset(const std::vector<double>& x, const std::vector<double>& y, const string xlabel, const string ylabel) noexcept 
    : xlabel(xlabel), xerrlabel(xlabel+"err"), ylabel(ylabel), yerrlabel(ylabel+"err"), x(x), y(y) {
    
    validate_sizes();
    plot_options.xlabel = xlabel;
    plot_options.ylabel = ylabel;
}

Dataset::Dataset(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& yerr) noexcept: x(x), y(y), yerr(yerr) {
    xerr.resize(yerr.size());
    validate_sizes();
}

Dataset::Dataset(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& xerr, const std::vector<double>& yerr) noexcept
    : x(x), xerr(xerr), y(y), yerr(yerr) {
        validate_sizes();
    }

void Dataset::reduce(unsigned int target, bool log) {
    if (size() < target) {throw except::invalid_operation("Error in Dataset::reduce: Target cannot be larger than the size of the data set.");}
    vector<double> new_x; new_x.reserve(target);
    vector<double> new_y; new_y.reserve(target);

    if (log) {
        double start = std::log10(x[0]); 
        double end = std::log10(x[x.size()-1]);
        double width = (end - start)/target;

        unsigned int j = 0;
        for (unsigned int i = 0; i < size(); i++) {
            double val = std::log10(x[i]);
            if (start + j*width < val) { // find the first x-value higher than our next sampling point
                new_x.push_back(x[i]);
                new_y.push_back(y[i]);
                j++;
            }
            while (start + j*width < val) { // it may be necessary to skip a few sampled points, especially at the beginning
                j++;
            }
        }
    } else {
        int ratio = std::floor(size()/target);
        for (unsigned int i = 0; i < size(); i++) {
            if (i % ratio == 0) {
                new_x.push_back(x[i]);
                new_y.push_back(y[i]);
            }
        }
    }

    x = std::move(new_x);
    y = std::move(new_y);
    plot_options.draw_line = false;
    plot_options.draw_markers = true;
}

std::size_t Dataset::size() const {return x.size();}

Dataset Dataset::copy() const {
    return Dataset(*this);
}

void Dataset::limit(const Limit& limits) {
    if (limits.min < x[0] && x[size()-1] < limits.max) {return;}

    vector<double> new_x; new_x.reserve(size());
    vector<double> new_y; new_y.reserve(size());

    for (unsigned int i = 0; i < size(); i++) {
        double val = x[i];
        if (val < limits.min || limits.max < val) {continue;}
        new_x.push_back(x[i]);
        new_y.push_back(y[i]);
    }

    x = std::move(new_x);
    y = std::move(new_y);
}

std::vector<double>& Dataset::get(const string label) {
    if (xlabel == label) {return x;}
    if (ylabel == label) {return y;}
    if (xerrlabel == label) {return xerr;}
    if (yerrlabel == label) {return yerr;}
    throw except::unknown_argument("Error in Dataset::get: No data is labelled as \"" + label + "\". Labels: " + xlabel + ", y: " + ylabel + ".");
}

void Dataset::validate_sizes() const {
    if (x.size() != y.size()
        || (has_yerr() && yerr.size() != x.size())
        || (has_xerr() && xerr.size() != x.size())) 
        {
            throw except::size_error("Error in Dataset::Dataset: x and y must have same size!");
    }
}

std::unique_ptr<TGraph> Dataset::plot() const {
    if (has_yerr()) {
        if (has_xerr()) {
            return std::make_unique<TGraphErrors>(x.size(), x.data(), y.data(), xerr.data(), yerr.data());
        }        
        return std::make_unique<TGraphErrors>(x.size(), x.data(), y.data(), nullptr, yerr.data());
    }
    else {
        return std::make_unique<TGraph>(x.size(), x.data(), y.data());}
}

void Dataset::scale_errors(double factor) {
    if (has_xerr()) {std::transform(xerr.begin(), xerr.end(), xerr.begin(), [&factor] (double val) {return factor*val;});}
    if (has_yerr()) {std::transform(yerr.begin(), yerr.end(), yerr.begin(), [&factor] (double val) {return factor*val;});}
}

void Dataset::scale_y(double factor) {
    std::transform(y.begin(), y.end(), y.begin(), [&factor] (double val) {return val*factor;});
    if (has_yerr()) {std::transform(yerr.begin(), yerr.end(), yerr.begin(), [&factor] (double val) {return factor*val;});}
}

void Dataset::normalize(double y0) {
    scale_y(y0/y[0]);
}

Dataset Dataset::generate_random_data(unsigned int size, double min, double max) {
    std::random_device dev;
    std::mt19937 gen(dev());
    auto uniform = std::uniform_real_distribution<double>(min, max);

    vector<double> x(size), y(size), yerr(size);
    for (unsigned int i = 0; i < size; i++) {
        x[i] = i;
        y[i] = uniform(gen);
        yerr[i] = y[i]*0.1;
    }
    return Dataset(x, y, yerr);
}

void Dataset::ylimits(double min, double max) noexcept {ylimits({min, max});}

void Dataset::ylimits(const Limit& limits) noexcept {
    std::vector<double> newx, newy, newxerr, newyerr;
    for (unsigned int i = 0; i < size(); i++) {
        if (limits.min <= y[i] && y[i] <= limits.max) {
            newx.push_back(x[i]);
            newy.push_back(y[i]);
            if (has_xerr()) {newxerr.push_back(xerr[i]);}
            if (has_yerr()) {newyerr.push_back(yerr[i]);}
        }
    }

    x = std::move(newx);
    y = std::move(newy);
    xerr = std::move(newxerr);
    yerr = std::move(newyerr);
}

void Dataset::xlimits(double min, double max) noexcept {xlimits({min, max});}

void Dataset::xlimits(const Limit& limits) noexcept {
    std::vector<double> newx, newy, newxerr, newyerr;
    for (unsigned int i = 0; i < size(); i++) {
        if (limits.min <= x[i] && x[i] <= limits.max) {
            newx.push_back(x[i]);
            newy.push_back(y[i]);
            if (has_xerr()) {newxerr.push_back(xerr[i]);}
            if (has_yerr()) {newyerr.push_back(yerr[i]);}
        }
    }

    x = std::move(newx);
    y = std::move(newy);
    xerr = std::move(newxerr);
    yerr = std::move(newyerr);
}

[[nodiscard]] Limit Dataset::span() const noexcept {
    auto[min, max] = std::minmax_element(y.begin(), y.end());
    return Limit(*min, *max);
}

[[nodiscard]] Limit Dataset::span_positive() const noexcept {
    if (size() == 0) {
        return Limit(0, 0);
    }

    Limit limits(y[0], y[0]);
    for (double val : y) {
        if (0 < val) {
            limits.min = std::min(val, limits.min);
        }
        limits.max = std::max(val, limits.max);
    }
    return limits;
}

void Dataset::simulate_noise() {
    if (!has_yerr()) {throw except::invalid_operation("Error in Dataset::simulate_noise: Cannot simulate noise without yerrs.");}

    std::random_device dev;
    std::mt19937 gen(dev());
    auto fun = [&] (double y, double yerr) {
        auto gauss = std::normal_distribution<double>(y, yerr);
        return gauss(gen);
    };

    std::transform(y.begin(), y.end(), yerr.begin(), y.begin(), fun);
}

void SAXSDataset::simulate_errors() {
    if (yerr.empty()) {yerr.resize(size());}
    else {utility::print_warning("Warning in Dataset::simulate_errors: Overwriting existing errors.");}
    if (xerr.empty()) {xerr.resize(size());}

    double y0 = y[0];
    // std::transform(y.begin(), y.end(), x.begin(), yerr.begin(), [&y0] (double y, double x) {return std::pow(y*x, 0.85);});
    // std::transform(y.begin(), y.end(), x.begin(), yerr.begin(), [&y0] (double y, double x) {return std::pow(y, 0.15)*std::pow(y0, 0.35)*std::pow(x, -0.85)/10000 + std::pow(x, 5)/100;});
    // std::transform(y.begin(), y.end(), x.begin(), yerr.begin(), [&y0] (double y, double x) {return y/x*1e-4 + 1e-4;});
    std::transform(y.begin(), y.end(), x.begin(), yerr.begin(), [&y0] (double y, double x) {return y0/std::pow(x, 1.2)*1e-5 + 1e-4*y0;});
}

void SAXSDataset::set_resolution(unsigned int resolution) {
    this->resolution = resolution;
    limit(Limit(0, 2*M_PI/resolution));
}

void Dataset::read(const string file) {
    // check if file was succesfully opened
    std::ifstream input(file);
    if (!input.is_open()) {throw std::ios_base::failure("Error in IntensityFitter::read: Could not open file \"" + file + "\"");}

    string line; // placeholder for the current line
    while(getline(input, line)) {
        if (line[0] == ' ') {line = line.substr(1);} // fix leading space
        vector<string> tokens;
        boost::split(tokens, line, boost::is_any_of(" ,\t")); // spaces, commas, and tabs can all be used as separators (but not a mix of them)

        // determine if we are in some sort of header
        if (tokens.size() < 2 || tokens.size() > 4) {continue;} // too many separators
        bool skip = false;
        for (unsigned int i = 0; i < tokens.size(); i++) { // check if they are numbers
            if (!tokens[i].empty() && tokens[i].find_first_not_of("0123456789-.Ee") != string::npos) {skip = true;}
        }
        if (skip) {continue;}

        // now we are most likely beyond any headers
        double _q, _I, _sigma;
        _q = std::stod(tokens[0]); // we know for sure that the strings are convertible to numbers (boost check)
        _I = std::stod(tokens[1]);
        if (_q > 10) {continue;} // probably not a q-value if it's larger than 10

        // check user-defined limits
        if (_q < setting::fit::q_low) {continue;}
        if (_q > setting::fit::q_high) {continue;}

        // add the values to our vectors
        x.push_back(_q);
        y.push_back(_I);

        // if x | y | yerr
        if (tokens.size() == 3) {
            _sigma = std::stod(tokens[2]);
            yerr.push_back(_sigma); 
        }
    }
    input.close();
}

bool Dataset::is_logarithmic() const noexcept {
    // generate a new dataset containing exp(Deltax) and fit it with linear regression.
    // if the fit is decent, the data must have been logaritmic
    Dataset exp_data;
    for (unsigned int i = 1; i < size(); i++) {
        exp_data.push_back({x[i], std::exp(x[i]-x[i-1]), 1});
    }

    SimpleLeastSquares fit(std::move(exp_data));
    auto res = fit.fit();

    std::cout << "DATASET IS_LOGARITHMIC FIT CHI: " << res->fval/res->dof << std::endl;
    std::cout << "chi: " << res->fval << std::endl;
    std::cout << "dof: " << res->dof << std::endl;
    return res->fval/res->dof < 10;
}

void Dataset::rebin() noexcept {
    Dataset data; // rebinned dataset

    for (unsigned int i = 0; i < size(); i++) {
        // determine how many data points to fold into one
        unsigned int fold;
        if (0.1 < x[i]) {fold = 8;}
        else if (0.06 < x[i]) {fold = 4;}
        else if (0.03 < x[i]) {fold = 2;}
        else {fold = 1;}

        std::cout << "now folding " << i << " to " << i + fold << std::endl;

        // loop over each data point to be folded
        double siginv = 0, sumw = 0, qsum = 0;
        unsigned int ss = 0;
        for (; ss < fold; ss++) {
            std::cout << "checkpoint1" << std::endl;
            if (i == size()) {break;}
            std::cout << "checkpoint1" << std::endl;
            siginv += (std::pow(yerr[i], -2));
            std::cout << "checkpoint1" << std::endl;
            sumw += y[i]/(std::pow(yerr[i], 2));
            std::cout << "checkpoint1" << std::endl;
            qsum += x[i];
            std::cout << "checkpoint1" << std::endl;
            i++;

        }

        // average their values into a single new one
        double q = qsum/ss;
        double I = sumw/siginv;
        double Ierr = std::pow(siginv, -0.5);
        data.push_back(Point2D(q, I, Ierr));
    }
    data.save("temp/dataset/test.dat");
    *this = data;
}

Point2D Dataset::get_point(unsigned int index) const noexcept {return Point2D(x[index], y[index]);}

Point2D Dataset::find_minimum() const noexcept {
    auto it = std::min_element(y.begin(), y.end());
    unsigned int index = it - y.begin();
    return get_point(index);
}

void Dataset::push_back(const Point2D& point) noexcept {
    x.push_back(point.x);
    y.push_back(point.y);
    if (has_xerr()) {xerr.push_back(point.xerr);}
    if (has_yerr()) {yerr.push_back(point.yerr);}
}

bool Dataset::has_xerr() const noexcept {return x.size() == xerr.size() && xerr.size() != 0 && xerr[0] != 0;}

bool Dataset::has_yerr() const noexcept {return y.size() == yerr.size() && yerr.size() != 0 && yerr[0] != 0;}