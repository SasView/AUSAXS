#pragma once

#include <hist/intensity_calculator/interface/ICompositeDistanceHistogram.h>
#include <hist/distribution/GenericDistribution1D.h>

#include <vector>

namespace hist {
    /**
     * @brief A class containing multiple partial distance histograms.
     */
    template<bool use_weighted_distribution>
    class CompositeDistanceHistogram : public ICompositeDistanceHistogram {
        public: 
            CompositeDistanceHistogram() = default;

            CompositeDistanceHistogram(
                typename hist::GenericDistribution1D<use_weighted_distribution>::type&& p_aa, 
                typename hist::GenericDistribution1D<use_weighted_distribution>::type&& p_wa, 
                typename hist::GenericDistribution1D<use_weighted_distribution>::type&& p_ww, 
                hist::Distribution1D&& p_tot, 
                const Axis& axis
            );

            virtual ~CompositeDistanceHistogram() override;

            /**
             * @brief Get the partial distance histogram for atom-atom interactions.
             */
            virtual const std::vector<constants::axes::d_type>& get_aa_counts() const override;
            virtual std::vector<constants::axes::d_type>& get_aa_counts() override; // @copydoc get_aa_counts() const

            /**
             * @brief Get the partial distance histogram for atom-water interactions.
             */
            virtual const std::vector<constants::axes::d_type>& get_aw_counts() const override;
            virtual std::vector<constants::axes::d_type>& get_aw_counts() override; // @copydoc get_aw_counts() const

            /**
             * @brief Get the partial distance histogram for water-water interactions.
             */
            virtual const std::vector<constants::axes::d_type>& get_ww_counts() const override;
            virtual std::vector<constants::axes::d_type>& get_ww_counts() override; // @copydoc get_ww_counts() const

            /**
             * @brief Apply a scaling factor to the water partial distance histogram.
             */
            virtual void apply_water_scaling_factor(double k) override;

            /**
             * @brief Reset the water scaling factor to 1.
             */
            void reset_water_scaling_factor();

            /**
             * @brief Get the intensity profile for atom-atom interactions.
             */
            virtual const ScatteringProfile get_profile_aa() const override;

            /**
             * @brief Get the intensity profile for atom-water interactions.
             */
            virtual const ScatteringProfile get_profile_aw() const override;

            /**
             * @brief Get the intensity profile for water-water interactions.
             */
            virtual const ScatteringProfile get_profile_ww() const override;

        private:
            mutable typename hist::GenericDistribution1D<use_weighted_distribution>::type p_aa;
            mutable typename hist::GenericDistribution1D<use_weighted_distribution>::type p_aw;
            mutable typename hist::GenericDistribution1D<use_weighted_distribution>::type p_ww;
    };
}