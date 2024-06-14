/*
This software is distributed under the GNU Lesser General Public License v3.0. 
For more information, please refer to the LICENSE file in the project root.
*/

#include <hist/intensity_calculator/CompositeDistanceHistogramFFExplicitBase.h>
#include <hist/Histogram.h>
#include <table/ArrayDebyeTable.h>
#include <form_factor/FormFactor.h>
#include <form_factor/PrecalculatedFormFactorProduct.h>
#include <form_factor/PrecalculatedExvFormFactorProduct.h>
#include <settings/HistogramSettings.h>
#include <constants/ConstantsMath.h>
#include <math/ConstexprMath.h>
#include <dataset/SimpleDataset.h>

using namespace hist;

template<typename AA, typename AXFormFactorTableType, typename XX>
CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::CompositeDistanceHistogramFFExplicitBase() = default;

template<typename AA, typename AX, typename XX>
CompositeDistanceHistogramFFExplicitBase<AA, AX, XX>::CompositeDistanceHistogramFFExplicitBase(CompositeDistanceHistogramFFExplicitBase&&) noexcept = default;

template<typename AA, typename AX, typename XX>
CompositeDistanceHistogramFFExplicitBase<AA, AX, XX>& CompositeDistanceHistogramFFExplicitBase<AA, AX, XX>::operator=(CompositeDistanceHistogramFFExplicitBase&&) noexcept = default;

template<typename AA, typename AXFormFactorTableType, typename XX>
CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::~CompositeDistanceHistogramFFExplicitBase() = default;

template<typename AA, typename AXFormFactorTableType, typename XX>
CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::CompositeDistanceHistogramFFExplicitBase(
    hist::Distribution3D&& p_aa, 
    hist::Distribution3D&& p_ax, 
    hist::Distribution3D&& p_xx, 
    hist::Distribution2D&& p_aw, 
    hist::Distribution2D&& p_wx, 
    hist::Distribution1D&& p_ww,
    hist::Distribution1D&& p_tot
) : CompositeDistanceHistogramFFAvgBase<AA>(std::move(p_aa), std::move(p_aw), std::move(p_ww), std::move(p_tot)), exv_distance_profiles{.xx=std::move(p_xx), .ax=std::move(p_ax), .wx=std::move(p_wx)} {}

template<typename AA, typename AXFormFactorTableType, typename XX>
CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::CompositeDistanceHistogramFFExplicitBase(
    hist::Distribution3D&& p_aa, 
    hist::Distribution3D&& p_ax, 
    hist::Distribution3D&& p_xx, 
    hist::Distribution2D&& p_aw, 
    hist::Distribution2D&& p_wx, 
    hist::Distribution1D&& p_ww, 
    hist::WeightedDistribution1D&& p_tot
) : CompositeDistanceHistogramFFAvgBase<AA>(std::move(p_aa), std::move(p_aw), std::move(p_ww), std::move(p_tot)), exv_distance_profiles{.xx=std::move(p_xx), .ax=std::move(p_ax), .wx=std::move(p_wx)} {}

template<typename AA, typename AXFormFactorTableType, typename XX>
const AA CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::get_ffaa_table() const {
    return this->get_ff_table();
}

template<typename AA, typename AXFormFactorTableType, typename XX>
double CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::exv_factor(double q) const {
    constexpr double rm2 = constants::radius::average_atomic_radius*constants::radius::average_atomic_radius;
    return std::pow(this->free_params.cx, 3)*std::exp(-rm2*(std::pow(this->free_params.cx, 2) - 1)*q*q/4);
}

template<typename AA, typename AXFormFactorTableType, typename XX>
ScatteringProfile CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::debye_transform() const {
    const auto& ff_aa_table = get_ffaa_table();
    const auto& ff_ax_table = get_ffax_table();
    const auto& ff_xx_table = get_ffxx_table();
    auto sinqd_table = this->get_sinc_table();

    // calculate the Debye scattering intensity
    Axis debye_axis = constants::axes::q_axis.sub_axis(settings::axes::qmin, settings::axes::qmax);
    unsigned int q0 = constants::axes::q_axis.get_bin(settings::axes::qmin); // account for a possibly different qmin

    std::vector<double> Iq(debye_axis.bins, 0);
    for (unsigned int q = q0; q < q0+debye_axis.bins; ++q) {
        double cx = exv_factor(constants::axes::q_vals[q]);
        for (unsigned int ff1 = 0; ff1 < form_factor::get_count_without_excluded_volume(); ++ff1) {
            for (unsigned int ff2 = 0; ff2 < form_factor::get_count_without_excluded_volume(); ++ff2) {
                // atom-atom
                double aa_sum = std::inner_product(this->distance_profiles.aa.begin(ff1, ff2), this->distance_profiles.aa.end(ff1, ff2), sinqd_table->begin(q), 0.0);
                Iq[q-q0] += aa_sum*ff_aa_table.index(ff1, ff2).evaluate(q);

                // atom-exv
                double ax_sum = std::inner_product(exv_distance_profiles.ax.begin(ff1, ff2), exv_distance_profiles.ax.end(ff1, ff2), sinqd_table->begin(q), 0.0);
                Iq[q-q0] -= 2*cx*ax_sum*ff_ax_table.index(ff1, ff2).evaluate(q);

                // exv-exv
                double xx_sum = std::inner_product(exv_distance_profiles.xx.begin(ff1, ff2), exv_distance_profiles.xx.end(ff1, ff2), sinqd_table->begin(q), 0.0);
                Iq[q-q0] += cx*cx*xx_sum*ff_xx_table.index(ff1, ff2).evaluate(q);
            }

            // atom-water
            double aw_sum = std::inner_product(this->distance_profiles.aw.begin(ff1), this->distance_profiles.aw.end(ff1), sinqd_table->begin(q), 0.0);
            Iq[q-q0] += 2*this->free_params.cw*aw_sum*ff_aa_table.index(ff1, form_factor::water_bin).evaluate(q);

            // exv-water
            double wx_sum = std::inner_product(exv_distance_profiles.wx.begin(ff1), exv_distance_profiles.wx.end(ff1), sinqd_table->begin(q), 0.0);
            Iq[q-q0] -= 2*cx*this->free_params.cw*wx_sum*ff_ax_table.index(form_factor::water_bin, ff1).evaluate(q);
        }

        // water-water
        double ww_sum = std::inner_product(this->distance_profiles.ww.begin(), this->distance_profiles.ww.end(), sinqd_table->begin(q), 0.0);
        Iq[q-q0] += std::pow(this->free_params.cw, 2)*ww_sum*ff_aa_table.index(form_factor::water_bin, form_factor::water_bin).evaluate(q);
    }
    return ScatteringProfile(std::move(Iq), debye_axis);
}

template<typename AA, typename AXFormFactorTableType, typename XX>
SimpleDataset CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::debye_transform(const std::vector<double>&) const {
    throw except::not_implemented("CompositeDistanceHistogramFFGrid::debye_transform(const std::vector<double>& q) const");
}

template<typename AA, typename AXFormFactorTableType, typename XX>
ScatteringProfile CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::get_profile_ax() const {
    const auto& ff_ax_table = get_ffax_table();
    auto sinqd_table = this->get_sinc_table();
    Axis debye_axis = constants::axes::q_axis.sub_axis(settings::axes::qmin, settings::axes::qmax);
    unsigned int q0 = constants::axes::q_axis.get_bin(settings::axes::qmin); // account for a possibly different qmin

    std::vector<double> Iq(debye_axis.bins, 0);
    for (unsigned int q = q0; q < q0+debye_axis.bins; ++q) {
        double cx = exv_factor(constants::axes::q_vals[q]);
        for (unsigned int ff1 = 0; ff1 < form_factor::get_count_without_excluded_volume(); ++ff1) {
            for (unsigned int ff2 = 0; ff2 < form_factor::get_count_without_excluded_volume(); ++ff2) {
                double ax_sum = std::inner_product(exv_distance_profiles.ax.begin(ff1, ff2), exv_distance_profiles.ax.end(ff1, ff2), sinqd_table->begin(q), 0.0);
                Iq[q-q0] += 2*cx*ax_sum*ff_ax_table.index(ff1, ff2).evaluate(q);
            }
        }
    }
    return ScatteringProfile(std::move(Iq), debye_axis);
}

template<typename AA, typename AXFormFactorTableType, typename XX>
ScatteringProfile CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::get_profile_xx() const {
    const auto& ff_xx_table = get_ffxx_table();
    auto sinqd_table = this->get_sinc_table();
    Axis debye_axis = constants::axes::q_axis.sub_axis(settings::axes::qmin, settings::axes::qmax);
    unsigned int q0 = constants::axes::q_axis.get_bin(settings::axes::qmin); // account for a possibly different qmin

    std::vector<double> Iq(debye_axis.bins, 0);
    for (unsigned int q = q0; q < q0+debye_axis.bins; ++q) {
        double cx2 = std::pow(exv_factor(constants::axes::q_vals[q]), 2);
        for (unsigned int ff1 = 0; ff1 < form_factor::get_count_without_excluded_volume(); ++ff1) {
            for (unsigned int ff2 = 0; ff2 < form_factor::get_count_without_excluded_volume(); ++ff2) {
                double xx_sum = std::inner_product(exv_distance_profiles.xx.begin(ff1, ff2), exv_distance_profiles.xx.end(ff1, ff2), sinqd_table->begin(q), 0.0);
                Iq[q-q0] += cx2*xx_sum*ff_xx_table.index(ff1, ff2).evaluate(q);
            }
        }
    }
    return ScatteringProfile(std::move(Iq), debye_axis);
}

template<typename AA, typename AXFormFactorTableType, typename XX>
ScatteringProfile CompositeDistanceHistogramFFExplicitBase<AA, AXFormFactorTableType, XX>::get_profile_wx() const {
    const auto& ff_ax_table = get_ffax_table();
    auto sinqd_table = this->get_sinc_table();
    Axis debye_axis = constants::axes::q_axis.sub_axis(settings::axes::qmin, settings::axes::qmax);
    unsigned int q0 = constants::axes::q_axis.get_bin(settings::axes::qmin); // account for a possibly different qmin

    std::vector<double> Iq(debye_axis.bins, 0);
    unsigned int ff_w_index = static_cast<int>(form_factor::form_factor_t::OH);
    for (unsigned int q = q0; q < q0+debye_axis.bins; ++q) {
        double cx = exv_factor(constants::axes::q_vals[q]);
        for (unsigned int ff1 = 0; ff1 < form_factor::get_count_without_excluded_volume(); ++ff1) {
            double wx_sum = std::inner_product(exv_distance_profiles.wx.begin(ff1), exv_distance_profiles.wx.end(ff1), sinqd_table->begin(q), 0.0);
            Iq[q-q0] += 2*cx*this->free_params.cw*wx_sum*ff_ax_table.index(ff_w_index, ff1).evaluate(q);
        }
    }
    return ScatteringProfile(std::move(Iq), debye_axis);
}

template class hist::CompositeDistanceHistogramFFExplicitBase<form_factor::storage::atomic::table_t, form_factor::storage::cross::table_t, form_factor::storage::exv::table_t>;