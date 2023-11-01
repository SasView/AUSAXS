#include <hist/distance_calculator/HistogramManagerMTFFGrid.h>
#include <hist/detail/CompactCoordinatesFF.h>
#include <hist/intensity_calculator/DistanceHistogram.h>
#include <hist/intensity_calculator/CompositeDistanceHistogramFFAvg.h>
#include <hist/intensity_calculator/CompositeDistanceHistogramFFGrid.h>
#include <data/Molecule.h>
#include <hydrate/Grid.h>
#include <settings/GeneralSettings.h>
#include <settings/GridSettings.h>
#include <constants/Constants.h>
#include <hist/distribution/WeightedDistanceDistribution1D.h>
#include <hist/distribution/WeightedDistanceDistribution2D.h>
#include <form_factor/FormFactorType.h>
#include <utility/MultiThreading.h>

using namespace hist;

HistogramManagerMTFFGrid::~HistogramManagerMTFFGrid() = default;

std::unique_ptr<DistanceHistogram> HistogramManagerMTFFGrid::calculate() {
    return calculate_all();
}

std::unique_ptr<ICompositeDistanceHistogram> HistogramManagerMTFFGrid::calculate_all() {
    using precision = float;

    constexpr unsigned int exv_bin = static_cast<unsigned int>(form_factor::form_factor_t::EXCLUDED_VOLUME);
    auto base_res = HistogramManagerMTFFAvg::calculate_all(); // make sure everything is initialized
    hist::detail::CompactCoordinates data_x = hist::detail::CompactCoordinates(protein->get_grid()->generate_excluded_volume());
    auto& data_p = *data_p_ptr;
    auto& data_h = *data_h_ptr;

    static double Z_exv = std::pow(settings::grid::width, 3)*constants::charge::density::water;
    static double Z_exv2 = Z_exv*Z_exv;

    //########################//
    // PREPARE MULTITHREADING //
    //########################//
    auto pool = utility::multi_threading::get_global_pool();
    auto calc_xx = [&data_x] (unsigned int imin, unsigned int imax) {
        hist::WeightedDistribution1D<precision> p_xx(constants::axes::d_axis.bins, 0);
        for (unsigned int i = imin; i < imax; ++i) { // exv
            unsigned int j = i+1;                    // exv
            for (; j+7 < data_x.get_size(); j+=8) {
                auto res = data_x[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3], data_x[j+4], data_x[j+5], data_x[j+6], data_x[j+7]);
                for (unsigned int k = 0; k < 8; ++k) {p_xx.add(res.distance[k], 2*Z_exv2);}
            }

            for (; j+3 < data_x.get_size(); j+=4) {
                auto res = data_x[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3]);
                for (unsigned int k = 0; k < 4; ++k) {p_xx.add(res.distance[k], 2*Z_exv2);}
            }

            for (; j < data_x.get_size(); ++j) {
                auto res = data_x[i].evaluate(data_x[j]);
                p_xx.add(res.distance, 2*Z_exv2);
            }
        }
        return p_xx;
    };

    auto calc_ax = [&data_p, &data_x] (unsigned int imin, unsigned int imax) {
        hist::WeightedDistribution2D<precision> p_ax(form_factor::get_count(), constants::axes::d_axis.bins, 0);
        for (unsigned int i = imin; i < imax; ++i) { // atoms
            unsigned int j = 0;                      // exv
            for (; j+7 < data_x.get_size(); j+=8) {
                auto res = data_p[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3], data_x[j+4], data_x[j+5], data_x[j+6], data_x[j+7]);
                for (unsigned int k = 0; k < 8; ++k) {p_ax.add(data_p.get_ff_type(i), res.distance[k], data_p[i].value.w*Z_exv);}
            }

            for (; j+3 < data_x.get_size(); j+=4) {
                auto res = data_p[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3]);
                for (unsigned int k = 0; k < 4; ++k) {p_ax.add(data_p.get_ff_type(i), res.distance[k], data_p[i].value.w*Z_exv);}
            }

            for (; j < data_x.get_size(); ++j) {
                auto res = data_p[i].evaluate(data_x[j]);
                p_ax.add(data_p.get_ff_type(i), res.distance, data_p[i].value.w*Z_exv);
            }
        }
        return p_ax;
    };

    auto calc_wx = [&data_h, &data_x] (unsigned int imin, unsigned int imax) {
        hist::WeightedDistribution1D<precision> p_wx(constants::axes::d_axis.bins, 0);
        for (unsigned int i = imin; i < imax; ++i) { // waters
            unsigned int j = 0;                      // exv
            for (; j+7 < data_x.get_size(); j+=8) {
                auto res = data_h[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3], data_x[j+4], data_x[j+5], data_x[j+6], data_x[j+7]);
                for (unsigned int k = 0; k < 8; ++k) {p_wx.add(res.distance[k], data_h[i].value.w*Z_exv);}
            }

            for (; j+3 < data_x.get_size(); j+=4) {
                auto res = data_h[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3]);
                for (unsigned int k = 0; k < 4; ++k) {p_wx.add(res.distance[k], data_h[i].value.w*Z_exv);}
            }

            for (; j < data_x.get_size(); ++j) {
                auto res = data_h[i].evaluate(data_x[j]);
                p_wx.add(res.distance, data_h[i].value.w*Z_exv);
            }
        }
        return p_wx;
    };

    //##############//
    // SUBMIT TASKS //
    //##############//
    unsigned int job_size = settings::general::detail::job_size;
    BS::multi_future<hist::WeightedDistribution1D<precision>> xx;
    for (unsigned int i = 0; i < protein->atom_size(); i+=job_size) {
        xx.push_back(pool->submit(calc_xx, i, std::min(i+job_size, (unsigned int)protein->atom_size())));
    }
    BS::multi_future<hist::WeightedDistribution2D<precision>> ax;
    for (unsigned int i = 0; i < protein->water_size(); i+=job_size) {
        ax.push_back(pool->submit(calc_ax, i, std::min(i+job_size, (unsigned int)protein->water_size())));
    }
    BS::multi_future<hist::WeightedDistribution1D<precision>> wx;
    for (unsigned int i = 0; i < protein->water_size(); i+=job_size) {
        wx.push_back(pool->submit(calc_wx, i, std::min(i+job_size, (unsigned int)protein->water_size())));
    }

    //#################//
    // COLLECT RESULTS //
    //#################//
    auto p_xx_future = pool->submit(
        [&](){
            hist::WeightedDistribution1D<precision> p_xx(constants::axes::d_axis.bins, 0);
            for (const auto& tmp : xx.get()) {
                std::transform(p_xx.begin(), p_xx.end(), tmp.begin(), p_xx.begin(), std::plus<precision>());
            }
            return p_xx;
        }
    );

    auto p_ax_future = pool->submit(
        [&](){
            hist::WeightedDistribution2D<precision> p_ax(form_factor::get_count(), constants::axes::d_axis.bins, 0);
            for (const auto& tmp : ax.get()) {
                std::transform(p_ax.begin(), p_ax.end(), tmp.begin(), p_ax.begin(), std::plus<precision>());
            }
            return p_ax;
        }
    );

    auto p_wx_future = pool->submit(
        [&](){
            hist::WeightedDistribution1D<precision> p_wx(constants::axes::d_axis.bins, 0);
            for (const auto& tmp : wx.get()) {
                std::transform(p_wx.begin(), p_wx.end(), tmp.begin(), p_wx.begin(), std::plus<precision>());
            }
            return p_wx;
        }
    );

    pool->wait_for_tasks();
    auto p_xx = p_xx_future.get();
    auto p_ax = p_ax_future.get();
    auto p_wx = p_wx_future.get();

    //###################//
    // SELF-CORRELATIONS //
    //###################//
    p_xx.add(0, 2*data_x.get_size());

    // downsize our axes to only the relevant area
    unsigned int max_bin = 10; // minimum size is 10
    for (int i = p_xx.size()-1; i >= 10; i--) {
        if (p_xx.index(i) != 0) {
            max_bin = i+1; // +1 since we usually use this for looping (i.e. i < max_bin)
            break;
        }
    }

    // ensure that our new vectors are compatible with those from the base class
    auto cast_res = static_cast<CompositeDistanceHistogramFFAvg*>(base_res.get());
    auto p_aa = std::move(cast_res->get_aa_counts_ff());
    auto p_aw = std::move(cast_res->get_aw_counts_ff());
    auto p_ww = std::move(cast_res->get_ww_counts_ff());

    if (max_bin > base_res->get_axis().bins) {
        p_aa.resize(max_bin);
        p_aw.resize(max_bin);
        p_ww.resize(max_bin);
    } else {
        max_bin = base_res->get_axis().bins; // make sure we overwrite anything which may already be stored
    }

    for (unsigned int i = 0; i < p_aa.size_x(); ++i) {
        std::move(p_ax.begin(i), p_ax.begin(i)+max_bin, p_aa.begin(i, exv_bin));
    }
    std::move(p_wx.begin(), p_wx.begin()+max_bin, p_aw.begin(exv_bin));
    std::move(p_xx.begin(), p_xx.begin()+max_bin, p_aa.begin(exv_bin, exv_bin));

    return std::make_unique<CompositeDistanceHistogramFFGrid>(
        std::move(p_aa), 
        std::move(p_aw), 
        std::move(p_ww), 
        Axis(0, max_bin*constants::axes::d_axis.width(), max_bin));
}