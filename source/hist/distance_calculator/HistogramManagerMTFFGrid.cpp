#include <hist/distance_calculator/HistogramManagerMTFFGrid.h>
#include <hist/detail/CompactCoordinatesFF.h>
#include <hist/DistanceHistogram.h>
#include <hist/CompositeDistanceHistogramFFAvg.h>
#include <data/Molecule.h>
#include <hydrate/Grid.h>
#include <settings/GeneralSettings.h>
#include <constants/Constants.h>
#include <container/Container3D.h>
#include <container/Container2D.h>
#include <container/Container1D.h>
#include <form_factor/FormFactorType.h>
#include <utility/MultiThreading.h>

using namespace hist;

HistogramManagerMTFFGrid::~HistogramManagerMTFFGrid() = default;

std::unique_ptr<DistanceHistogram> HistogramManagerMTFFGrid::calculate() {

}

std::unique_ptr<CompositeDistanceHistogram> HistogramManagerMTFFGrid::calculate_all() {
    constexpr unsigned int exv_bin = static_cast<unsigned int>(form_factor::form_factor_t::EXCLUDED_VOLUME);
    auto base_res = HistogramManagerMTFFAvg::calculate_all(); // make sure everything is initialized
    hist::detail::CompactCoordinates data_x = hist::detail::CompactCoordinates(protein->get_grid()->generate_excluded_volume());
    auto& data_p = *data_p_ptr;
    auto& data_h = *data_h_ptr;

    //########################//
    // PREPARE MULTITHREADING //
    //########################//
    auto pool = utility::multi_threading::get_global_pool();
    auto calc_xx = [&data_x] (unsigned int imin, unsigned int imax) {
        container::Container1D<double> p_xx(constants::axes::d_axis.bins, 0);
        for (unsigned int i = imin; i < imax; ++i) { // exv
            unsigned int j = 0;                      // exv
            for (; j+7 < data_x.get_size(); j+=8) {
                auto res = data_x[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3], data_x[j+4], data_x[j+5], data_x[j+6], data_x[j+7]);
                for (unsigned int k = 0; k < 8; ++k) {p_xx.index(res.distance[k]) += 2*res.weight[k];}
            }

            for (; j+3 < data_x.get_size(); j+=4) {
                auto res = data_x[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3]);
                for (unsigned int k = 0; k < 4; ++k) {p_xx.index(res.distance[k]) += 2*res.weight[k];}
            }

            for (; j < data_x.get_size(); ++j) {
                auto res = data_x[i].evaluate(data_x[j]);
                p_xx.index(res.distance) += 2*res.weight;
            }
        }
        return p_xx;
    };

    auto calc_ax = [&data_p, &data_x] (unsigned int imin, unsigned int imax) {
        container::Container2D<double> p_ax(constants::axes::d_axis.bins, 0);
        for (unsigned int i = imin; i < imax; ++i) { // atoms
            unsigned int j = 0;                      // exv
            for (; j+7 < data_x.get_size(); j+=8) {
                auto res = data_p[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3], data_x[j+4], data_x[j+5], data_x[j+6], data_x[j+7]);
                for (unsigned int k = 0; k < 8; ++k) {p_ax.index(data_p.get_ff_type(i), res.distance[k]) += 2*res.weight[k];}
            }

            for (; j+3 < data_x.get_size(); j+=4) {
                auto res = data_p[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3]);
                for (unsigned int k = 0; k < 4; ++k) {p_ax.index(data_p.get_ff_type(i), res.distance[k]) += 2*res.weight[k];}
            }

            for (; j < data_x.get_size(); ++j) {
                auto res = data_p[i].evaluate(data_x[j]);
                p_ax.index(data_p.get_ff_type(i), res.distance) += 2*res.weight;
            }
        }
        return p_ax;
    };

    auto calc_wx = [&data_h, &data_x] (unsigned int imin, unsigned int imax) {
        container::Container1D<double> p_wx(constants::axes::d_axis.bins, 0);
        for (unsigned int i = imin; i < imax; ++i) { // waters
            unsigned int j = 0;                      // exv
            for (; j+7 < data_x.get_size(); j+=8) {
                auto res = data_h[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3], data_x[j+4], data_x[j+5], data_x[j+6], data_x[j+7]);
                for (unsigned int k = 0; k < 8; ++k) {p_wx.index(res.distance[k]) += res.weight[k];}
            }

            for (; j+3 < data_x.get_size(); j+=4) {
                auto res = data_h[i].evaluate(data_x[j], data_x[j+1], data_x[j+2], data_x[j+3]);
                for (unsigned int k = 0; k < 4; ++k) {p_wx.index(res.distance[k]) += res.weight[k];}
            }

            for (; j < data_x.get_size(); ++j) {
                auto res = data_h[i].evaluate(data_x[j]);
                p_wx.index(res.distance) += res.weight;
            }
        }
        return p_wx;
    };

    //##############//
    // SUBMIT TASKS //
    //##############//
    unsigned int job_size = settings::general::detail::job_size;
    BS::multi_future<container::Container1D<double>> xx;
    for (unsigned int i = 0; i < protein->atom_size(); i+=job_size) {
        xx.push_back(pool->submit(calc_xx, i, std::min(i+job_size, (unsigned int)protein->atom_size())));
    }
    BS::multi_future<container::Container2D<double>> ax;
    for (unsigned int i = 0; i < protein->water_size(); i+=job_size) {
        ax.push_back(pool->submit(calc_ax, i, std::min(i+job_size, (unsigned int)protein->water_size())));
    }
    BS::multi_future<container::Container1D<double>> wx;
    for (unsigned int i = 0; i < protein->water_size(); i+=job_size) {
        wx.push_back(pool->submit(calc_wx, i, std::min(i+job_size, (unsigned int)protein->water_size())));
    }

    //#################//
    // COLLECT RESULTS //
    //#################//
    auto p_xx_future = pool->submit(
        [&](){
            container::Container1D<double> p_xx(constants::axes::d_axis.bins, 0);
            for (const auto& tmp : xx.get()) {
                std::transform(p_xx.begin(), p_xx.end(), tmp.begin(), p_xx.begin(), std::plus<double>());
            }
            return p_xx;
        }
    );

    auto p_ax_future = pool->submit(
        [&](){
            container::Container1D<double> p_ax(constants::axes::d_axis.bins, 0);
            for (const auto& tmp : ax.get()) {
                std::transform(p_ax.begin(), p_ax.end(), tmp.begin(), p_ax.begin(), std::plus<double>());
            }
            return p_ax;
        }
    );

    auto p_wx_future = pool->submit(
        [&](){
            container::Container1D<double> p_wx(constants::axes::d_axis.bins, 0);
            for (const auto& tmp : wx.get()) {
                std::transform(p_wx.begin(), p_wx.end(), tmp.begin(), p_wx.begin(), std::plus<double>());
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
    p_xx.index(0) = 2*data_x.get_size();

    // downsize our axes to only the relevant area
    unsigned int max_bin = 10; // minimum size is 10
    for (int i = p_xx.size()-1; i >= 10; i--) {
        if (p_xx.index(i) != 0) {
            max_bin = i+1; // +1 since we usually use this for looping (i.e. i < max_bin)
            break;
        }
    }

    // ensure that our new vectors are compatible with those from the base class
    unsigned int base_max_bin = base_res->get_axis().bins;
    auto cast_res = static_cast<CompositeDistanceHistogramFFAvg*>(base_res.get());
    auto p_aa = std::move(cast_res->get_aa_counts_ff());
    auto p_aw = std::move(cast_res->get_aw_counts_ff());
    auto p_ww = std::move(cast_res->get_ww_counts_ff());
    auto p_tot = std::move(cast_res->get_total_counts());

    if (max_bin > base_max_bin) {
        p_aa.resize(max_bin);
        p_aw.resize(max_bin);
        p_ww.resize(max_bin);
        p_tot.resize(max_bin);
    }

    for (unsigned int i = 0; i < p_aa.size_x(); ++i) {
        std::move(p_ax.begin(), p_ax.begin()+max_bin, p_aa.begin(i, exv_bin));
        std::move(p_wx.begin(), p_wx.begin()+max_bin, p_aw.begin(exv_bin));
    }
    std::move(p_xx.begin(), p_xx.begin()+max_bin, p_aa.begin(exv_bin, exv_bin));

    return std::make_unique<CompositeDistanceHistogramFFAvg>(
        std::move(p_aa), 
        std::move(p_aw), 
        std::move(p_ww), 
        std::move(p_tot), 
        Axis(0, max_bin*constants::axes::d_axis.width(), max_bin));
}