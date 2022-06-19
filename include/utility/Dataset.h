#pragma once

#include <vector>
#include <string>
#include <memory>
#include <any>

#include <TGraph.h>

#include <utility/Axis.h>
#include <plots/PlotOptions.h>
#include <utility/PointSet.h>

/**
 * @brief A representation of a set of 2D data. 
 */
class Dataset {
    public: 
        /**
         * @brief Default constructor.
         */
        Dataset() noexcept {}

        /**
         * @brief Constructor.
         * 
         * Create a new dataset based on an input file. Format is assumed to be x | y | yerr
         * 
         * @param file 
         */
        Dataset(const std::string file);

        /**
         * @brief Constructor. 
         * 
         * Create a new empty dataset with the given labels.
         */
        Dataset(std::string xlabel, std::string ylabel) noexcept;

        /**
         * @brief Constructor. 
         * 
         * Create a new dataset based on a list of x and y coordinates. 
         */
        Dataset(const std::vector<double>& x, const std::vector<double>& y) noexcept;

        /**
         * @brief Constructor. 
         * 
         * Create a new dataset based on a list of x and y coordinates, along with an error on the latter. 
         */
        Dataset(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& yerr) noexcept;

        /**
         * @brief Constructor. 
         * 
         * Create a new dataset based on a list of x and y coordinates, along with errors for both.
         */
        Dataset(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& yerr, const std::vector<double>& xerr) noexcept;

        /**
         * @brief Constructor. 
         * 
         * Create a new labelled dataset based on a list of x and y coordinates. 
         */
        Dataset(const std::vector<double>& x, const std::vector<double>& y, std::string xlabel, std::string ylabel) noexcept;

        /**
         * @brief Destructor. 
         */
        ~Dataset() = default;

        /**
         * @brief Reduce the number of data points to the specified amount. 
         * 
         * @return The modified dataset. 
         */
        void reduce(unsigned int target, bool log = false);

        /**
         * @brief Impose limits on the data. All points with an x-value outside this range will be removed. 
         * 
         * @param limits The new limits. 
         * @return The modified dataset. 
         */
        void limit(const Limit& limits);

        /**
         * @brief Get the number of data points. 
         */
        std::size_t size() const;

        /**
         * @brief Get a set of data based on its label. 
         * This also serves as a consistency check. 
         * 
         * @param label The name of the data. 
         */
        std::vector<double>& get(const std::string label);

        /**
         * @brief Check if the data is logarithmic. 
         *        This may be wrong if the x-data is very noisy.
         */
        bool is_logarithmic() const noexcept;

        /**
         * @brief Scale all errors by some common factor. 
         */
        void scale_errors(double factor);

        /**
         * @brief Scale the y-values (and their associated errors) by some common factor.
         */
        void scale_y(double factor);

        /**
         * @brief Set the normalization of the y-values. The first y-value will be fixed to this. 
         */
        void normalize(double y0);

        /**
         * @brief Plot this dataset.
         */
        std::unique_ptr<TGraph> plot() const;

        /**
         * @brief Overwrite the plot options for this dataset.
         */
        void set_plot_options(const plots::PlotOptions& options);

        /**
         * @brief Change the plot options for this dataset.
         */
        void add_plot_options(const std::map<std::string, std::any>& options);

        /**
         * @brief Change the plot options for this dataset.
         * 
         * @param style The plotting style. Should be one of the accepted variations of "markers", "errors", or "line". 
         * @param options The other plot options.
         */
        void add_plot_options(std::string style, std::map<std::string, std::any> options = {});

        /**
         * @brief Change the plot options for this dataset.
         * 
         * @param color The color.
         * @param options The other plot options.
         */
        void add_plot_options(int color, std::map<std::string, std::any> options = {});

        /**
         * @brief Write this dataset to the specified file. 
         */
        void save(std::string path) const;

        /**
         * @brief Simulate Gaussian noise on the y-values based on the errors. 
         */
        void simulate_noise();

        /**
         * @brief Create a copy of this Dataset.
         */
        Dataset copy() const;

        /**
         * @brief Get the point at a given index.
         */
        Point2D get_point(unsigned int index) const noexcept;

        /**
         * @brief Get the point with the smallest y-value.
         */
        Point2D find_minimum() const noexcept;

        /**
         * @brief Add a new datapoint to the end of this dataset. 
         */
        void push_back(const Point2D& point) noexcept;

        /**
         * @brief Check if this dataset has errors on the x-values.
         */
        [[nodiscard]] bool has_xerr() const noexcept;

        /**
         * @brief Check if this dataset has errors on the y-values.
         */
        [[nodiscard]] bool has_yerr() const noexcept;

        /**
         * @brief Rebin the data to a logarithmic scale. 
         *        This follows the typical rebinning algorithm used experimentally.
         */
        void rebin() noexcept;

        /**
         * @brief Generate a randomized dataset.
         * 
         * @param size Size of the dataset.
         * @param min Minimum generated value.
         * @param max Maxium generated value. 
         */
        static Dataset generate_random_data(unsigned int size, double min = 0, double max = 1);

        /**
         * @brief Impose a limit on the y-axis. All data lying outside this range will be removed.
         */
        void ylimits(double min, double max) noexcept;

        /**
         * @brief Impose a limit on the y-axis. All data lying outside this range will be removed.
         */
        void ylimits(const Limit& limit) noexcept;

        /**
         * @brief Get the spanned y-range. 
         */
        [[nodiscard]] Limit span() const noexcept;

        /**
         * @brief Get the positive spanned y-range.
         *        This can be useful for setting log ranges. 
         */
        [[nodiscard]] Limit span_positive() const noexcept;

        std::string xlabel = "x", xerrlabel = "xerr";
        std::string ylabel = "y", yerrlabel = "yerr";
        std::vector<double> x, xerr;
        std::vector<double> y, yerr;
        plots::PlotOptions plot_options; 

    private:
        /**
         * @brief Validate the sizes of the stored vectors. 
         */
        void validate_sizes() const;

        /**
         * @brief Read a dataset from a file.
         * 
         * @param file Path to the input file. 
         */
        void read(const std::string file);
};

class SAXSDataset : public Dataset {
    using Dataset::Dataset; // inherit constructors

    public:
        /**
         * @brief Generate errors for the y-values mimicking what one would find experimentally. 
         */
        void simulate_errors();

        /**
         * @brief Set the resolution of this dataset. 
         */
        void set_resolution(unsigned int resolution);

    private:
        unsigned int resolution = 0;
};