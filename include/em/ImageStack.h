#pragma once

#include <vector>

#include <em/CullingStrategy.h>
#include <em/datatypes.h>
#include <em/Image.h>
#include <data/Protein.h>
#include <hydrate/Grid.h>
#include <ScatteringHistogram.h>
#include <fitter/SimpleIntensityFitter.h>

using std::vector, std::list;

namespace em {
    /**
     * @brief \class ImageStack
     * 
     * A representation of a stack of images. 
     */
    class ImageStack {
        public:
            class EMFit : public Fitter::Fit {using Fitter::Fit::Fit;};

            /**
             * @brief Constructor.
             * 
             * @param file Path to the input EM data file. 
             */
            ImageStack(string file, unsigned int resolution = 0, setting::em::CullingStrategyChoice csc = setting::em::CullingStrategyChoice::CounterStrategy);

            /**
             * @brief Constructor.
             * 
             * @param resolution 
             */
            ImageStack(const vector<Image>& images, unsigned int resolution = 0, setting::em::CullingStrategyChoice csc = setting::em::CullingStrategyChoice::CounterStrategy);

            /**
             * @brief Destructor.
             */
            ~ImageStack() = default;

            /**
             * @brief Fit the cutoff value with the input experimental data file. 
             * 
             * @param filename Path to the measurement file. 
             */
            std::shared_ptr<EMFit> fit(string filename);

            /**
             * @brief Fit the cutoff value with the input histogram. 
             * 
             * @param h The histogram to fit to.  
             */
            std::shared_ptr<EMFit> fit(const ScatteringHistogram& h);

            /**
             * @brief Get a specific Image stored in this object. 
             * 
             * @param layer The vertical location of the Image. 
             */
            Image& image(unsigned int layer);

            /**
             * @brief Get a specific Image stored in this object. 
             * 
             * @param layer The vertical location of the Image. 
             */
            const Image& image(unsigned int layer) const;

            /**
             * @brief Prepare a ScatteringHistogram based on this object. 
             */
            ScatteringHistogram get_histogram(double cutoff) const;

            /**
             * @brief Get the fitted ScatteringHistogram.
             */
            ScatteringHistogram get_histogram(const std::shared_ptr<EMFit> res) const;

            /**
             * @brief Create a new Grid based on this object. 
             * 
             * @param cutoff The cutoff value. If positive, atoms will be generated at all pixel values higher than this. If negative, they will be generated at pixels lower than this. 
             */
            std::unique_ptr<Grid> create_grid(double cutoff) const;

            /**
             * @brief Create a new Protein based on this object. 
             * 
             * @param cutoff The cutoff value. If positive, atoms will be generated at all pixel values higher than this. If negative, they will be generated at pixels lower than this. 
             */
            std::unique_ptr<Protein> create_protein(double cutoff) const;

            /**
             * @brief Get the header of the input file. 
             */
            std::shared_ptr<ccp4::Header> get_header() const;

            /**
             * @brief Get the number of images stored in this object.
             */
            size_t size() const;

            /**
             * @brief Get a reference to all images stored in this object. 
             */
            const vector<Image>& images() const;

            /**
             * @brief Save this structure as a .pdb file. 
             * 
             * @param path Path to save location.
             * @param cutoff The cutoff value. If positive, atoms will be generated at all pixel values higher than this. If negative, they will be generated at pixels lower than this. 
             */
            void save(string path, double cutoff) const;

            /**
             * @brief Determine if this map is positively stained. 
             * 
             * Calculates the total mean density of this imagestack, and returns true if its sign is positive. 
             * Otherwise it is assumed to be negatively stained.
             */
            bool is_positively_stained() const;

            /**
             * @brief Get the limits on the q-values generated by discretizing the model scattering curve.
             * 
             * More specifically the upper limit will be $q_{max} = \frac{2\pi}{d}$ where $d$ is the resolution assumed to be the only number present in the filename. 
             */
            Limit get_limits() const;

            /**
             * @brief Get the mean density.
             */
            double mean() const;

            ObjectBounds3D minimum_volume(double cutoff);

        private:
            std::string filename;
            std::shared_ptr<ccp4::Header> header;
            vector<Image> data;
            std::unique_ptr<em::CullingStrategy> culler;
            unsigned int resolution;
            int staining = 0; // 0 if not determined yet, -1 if negatively stained, +1 if positively stained
            unsigned int size_x, size_y, size_z;

            /**
             * @brief Determines the minimum bounds necessariy to describe the map for the given cutoff.
             */
            void determine_minimum_bounds();

            /**
             * @brief Determine what type of staining (positive or negative) was used by analyzing the map.
             */
            void determine_staining();

            void read(std::ifstream& istream, size_t byte_size);

            void setup(setting::em::CullingStrategyChoice csc);

            /**
             * @brief Get the data byte size of the CCP file. 
             */
            size_t get_byte_size() const;

            /**
             * @brief A helper function for the fitting methods. This performs the actual fit. 
             * 
             * @param fitter The fitter object to fit. 
             */
            std::shared_ptr<EMFit> fit_helper(SimpleIntensityFitter& fitter);

            float& index(unsigned int x, unsigned int y, unsigned int z);

            float index(unsigned int x, unsigned int y, unsigned int z) const;
    };
}