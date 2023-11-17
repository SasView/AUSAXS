#include <hist/distance_calculator/HistogramManagerMTFFGrid.h>
#include <hist/detail/CompactCoordinatesFF.h>
#include <hist/intensity_calculator/DistanceHistogram.h>
#include <hist/intensity_calculator/CompositeDistanceHistogramFFAvg.h>
#include <hist/intensity_calculator/CompositeDistanceHistogramFFGrid.h>
#include <data/Molecule.h>
#include <hydrate/Grid.h>
#include <settings/GeneralSettings.h>
#include <settings/GridSettings.h>
#include <settings/HistogramSettings.h>
#include <constants/Constants.h>
#include <hist/distance_calculator/detail/TemplateHelpersFFAvg.h>
#include <hist/distribution/WeightedDistribution.h>
#include <form_factor/FormFactorType.h>
#include <utility/MultiThreading.h>

using namespace hist;

// custom evaluates for the grid since we don't want to account for the excluded volume
namespace grid {
    template<bool use_weighted_distribution, int factor>
    inline void evaluate8(typename hist::GenericDistribution2D<use_weighted_distribution>::type& p, const hist::detail::CompactCoordinatesFF& data_i, const hist::detail::CompactCoordinates& data_j, int i, int j) {
        auto res = ::detail::add8::evaluate<use_weighted_distribution>(data_i, data_j, i, j);
        for (unsigned int k = 0; k < 8; ++k) {p.add(data_i.get_ff_type(i), res.distances[k], factor*res.weights[k]);}
    }

    template<bool use_weighted_distribution, int factor>
    inline void evaluate4(typename hist::GenericDistribution2D<use_weighted_distribution>::type& p, const hist::detail::CompactCoordinatesFF& data_i, const hist::detail::CompactCoordinates& data_j, int i, int j) {
        auto res = ::detail::add4::evaluate<use_weighted_distribution>(data_i, data_j, i, j);
        for (unsigned int k = 0; k < 4; ++k) {p.add(data_i.get_ff_type(i), res.distances[k], factor*res.weights[k]);}
    }

    template<bool use_weighted_distribution, int factor>
    inline void evaluate1(typename hist::GenericDistribution2D<use_weighted_distribution>::type& p, const hist::detail::CompactCoordinatesFF& data_i, const hist::detail::CompactCoordinates& data_j, int i, int j) {
        auto res = ::detail::add1::evaluate<use_weighted_distribution>(data_i, data_j, i, j);
        p.add(data_i.get_ff_type(i), res.distance, factor*res.weight);
    }
}

template<bool use_weighted_distribution> 
HistogramManagerMTFFGrid<use_weighted_distribution>::~HistogramManagerMTFFGrid() = default;

template<bool use_weighted_distribution> 
std::unique_ptr<DistanceHistogram> HistogramManagerMTFFGrid<use_weighted_distribution>::calculate() {
    return calculate_all();
}

template<bool use_weighted_distribution> 
std::unique_ptr<ICompositeDistanceHistogram> HistogramManagerMTFFGrid<use_weighted_distribution>::calculate_all() {
    using GenericDistribution1D_t = typename hist::GenericDistribution1D<use_weighted_distribution>::type;
    using GenericDistribution2D_t = typename hist::GenericDistribution2D<use_weighted_distribution>::type;
    using GenericDistribution3D_t = typename hist::GenericDistribution3D<use_weighted_distribution>::type;
    auto pool = utility::multi_threading::get_global_pool();

    auto base_res = HistogramManagerMTFFAvg<use_weighted_distribution>::calculate_all(); // make sure everything is initialized
    hist::WeightedDistribution::reset();
    hist::detail::CompactCoordinates data_x = hist::detail::CompactCoordinates(this->protein->get_grid()->generate_excluded_volume(), 1);
    auto& data_a = *this->data_a_ptr;
    auto& data_w = *this->data_w_ptr;
    int data_a_size = (int) data_a.size();
    int data_w_size = (int) data_w.size();
    int data_x_size = (int) data_x.size();

    //########################//
    // PREPARE MULTITHREADING //
    //########################//
    auto calc_xx = [&data_x, data_x_size] (int imin, int imax) {
        GenericDistribution1D_t p_xx(constants::axes::d_axis.bins, 0);
        for (int i = imin; i < imax; ++i) { // exv
            int j = i+1;                    // exv
            for (; j+7 < data_x_size; j+=8) {
                evaluate8<use_weighted_distribution, 2>(p_xx, data_x, data_x, i, j);
            }

            for (; j+3 < data_x_size; j+=4) {
                evaluate4<use_weighted_distribution, 2>(p_xx, data_x, data_x, i, j);
            }

            for (; j < data_x_size; ++j) {
                evaluate1<use_weighted_distribution, 2>(p_xx, data_x, data_x, i, j);
            }
        }
        return p_xx;
    };

    auto calc_ax = [&data_a, &data_x, data_x_size] (int imin, int imax) {
        GenericDistribution2D_t p_ax(form_factor::get_count(), constants::axes::d_axis.bins, 0);
        for (int i = imin; i < imax; ++i) { // atoms
            int j = 0;                      // exv
            for (; j+7 < data_x_size; j+=8) {
                grid::evaluate8<use_weighted_distribution, 1>(p_ax, data_a, data_x, i, j);
            }

            for (; j+3 < data_x_size; j+=4) {
                grid::evaluate4<use_weighted_distribution, 1>(p_ax, data_a, data_x, i, j);
            }

            for (; j < data_x_size; ++j) {
                grid::evaluate1<use_weighted_distribution, 1>(p_ax, data_a, data_x, i, j);
            }
        }
        return p_ax;
    };

    auto calc_wx = [&data_w, &data_x, data_x_size] (int imin, int imax) {
        GenericDistribution1D_t p_wx(constants::axes::d_axis.bins, 0);
        for (int i = imin; i < imax; ++i) { // waters
            int j = 0;                      // exv
            for (; j+7 < data_x_size; j+=8) {
                evaluate8<use_weighted_distribution, 1>(p_wx, data_w, data_x, i, j);
            }

            for (; j+3 < data_x_size; j+=4) {
                evaluate4<use_weighted_distribution, 1>(p_wx, data_w, data_x, i, j);
            }

            for (; j < data_x_size; ++j) {
                evaluate1<use_weighted_distribution, 1>(p_wx, data_w, data_x, i, j);
            }
        }
        return p_wx;
    };

    //##############//
    // SUBMIT TASKS //
    //##############//
    int job_size = settings::general::detail::job_size;
    BS::multi_future<GenericDistribution1D_t> xx;
    for (int i = 0; i < (int) data_x_size; i+=job_size) {
        xx.push_back(pool->submit(calc_xx, i, std::min<int>(i+job_size, (int) data_x_size)));
    }
    BS::multi_future<GenericDistribution2D_t> ax;
    for (int i = 0; i < (int) data_a_size; i+=job_size) {
        ax.push_back(pool->submit(calc_ax, i, std::min<int>(i+job_size, (int) data_a_size)));
    }
    BS::multi_future<GenericDistribution1D_t> wx;
    for (int i = 0; i < (int) data_w_size; i+=job_size) {
        wx.push_back(pool->submit(calc_wx, i, std::min<int>(i+job_size, (int) data_w_size)));
    }

    //#################//
    // COLLECT RESULTS //
    //#################//
    auto p_xx_future = pool->submit(
        [&](){
            GenericDistribution1D_t p_xx(constants::axes::d_axis.bins, 0);
            for (const auto& tmp : xx.get()) {
                std::transform(p_xx.begin(), p_xx.end(), tmp.begin(), p_xx.begin(), std::plus<>());
            }
            return p_xx;
        }
    );

    auto p_ax_future = pool->submit(
        [&](){
            GenericDistribution2D_t p_ax(form_factor::get_count(), constants::axes::d_axis.bins, 0);
            for (const auto& tmp : ax.get()) {
                std::transform(p_ax.begin(), p_ax.end(), tmp.begin(), p_ax.begin(), std::plus<>());
            }
            return p_ax;
        }
    );

    auto p_wx_future = pool->submit(
        [&](){
            GenericDistribution1D_t p_wx(constants::axes::d_axis.bins, 0);
            for (const auto& tmp : wx.get()) {
                std::transform(p_wx.begin(), p_wx.end(), tmp.begin(), p_wx.begin(), std::plus<>());
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
    p_xx.add(0, data_x_size);

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
    GenericDistribution3D_t p_aa = std::move(cast_res->get_aa_counts_ff());
    GenericDistribution2D_t p_aw = std::move(cast_res->get_aw_counts_ff());
    GenericDistribution1D_t p_ww = std::move(cast_res->get_ww_counts_ff());

    if (base_res->get_axis().bins < max_bin) {
        p_aa.resize(max_bin);
        p_aw.resize(max_bin);
        p_ww.resize(max_bin);
    } else {
        max_bin = base_res->get_axis().bins; // make sure we overwrite anything which may already be stored
    }

    for (unsigned int i = 0; i < p_aa.size_x(); ++i) {
        std::move(p_ax.begin(i), p_ax.begin(i)+max_bin, p_aa.begin(i, form_factor::exv_bin));
    }
    std::move(p_wx.begin(), p_wx.begin()+max_bin, p_aw.begin(form_factor::exv_bin));
    std::move(p_xx.begin(), p_xx.begin()+max_bin, p_aa.begin(form_factor::exv_bin, form_factor::exv_bin));

    return std::make_unique<CompositeDistanceHistogramFFGrid>(
        std::move(p_aa), 
        std::move(p_aw), 
        std::move(p_ww), 
        Axis(0, max_bin*constants::axes::d_axis.width(), max_bin)
    );
}

template class hist::HistogramManagerMTFFGrid<false>;
template class hist::HistogramManagerMTFFGrid<true>;