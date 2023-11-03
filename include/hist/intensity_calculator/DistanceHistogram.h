#pragma once

#include <utility/Axis.h>
#include <hist/distribution/Distribution1D.h>
#include <hist/Histogram.h>
#include <hist/HistFwd.h>
#include <table/TableFwd.h>
#include <dataset/DatasetFwd.h>
#include <constants/Constants.h>

#include <vector>
#include <memory>

namespace hist {
    /**
     * @brief A DistanceHistogram is just a (x, count(x)) histogram.
     */
    class DistanceHistogram : public Histogram {
        public: 
            DistanceHistogram();
            DistanceHistogram(hist::Distribution1D&& p_tot, const Axis& axis);

            /**
             * @brief Extract the total histogram from a CompositeDistanceHistogram.
             */
            DistanceHistogram(std::unique_ptr<ICompositeDistanceHistogram> cdh);

            virtual ~DistanceHistogram() override;

            virtual ScatteringProfile debye_transform() const;

            // virtual SimpleDataset debye_transform(const std::vector<double>& q) const;

            const std::vector<double>& get_d_axis() const;

            const std::vector<double>& get_q_axis() const;

            /**
             * @brief Get the total histogram counts. Equivalent to get_counts().
             */
            virtual const std::vector<double>& get_total_counts() const;

            /**
             * @brief Get the total histogram counts. Equivalent to get_counts().
             */
            std::vector<double>& get_total_counts();

        protected:
            std::vector<double> d_axis; // the distance axis
            std::vector<double> q_axis; // the q axis

        private:
            void initialize();
    };
}