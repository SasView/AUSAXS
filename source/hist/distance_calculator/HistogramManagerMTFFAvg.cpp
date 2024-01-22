// #include <hist/distance_calculator/HistogramManagerMTFFAvg.h>
// #include <hist/intensity_calculator/CompositeDistanceHistogramFFAvg.h>
// #include <hist/distribution/GenericDistribution1D.h>
// #include <hist/distribution/GenericDistribution2D.h>
// #include <hist/distribution/GenericDistribution3D.h>
// #include <hist/distance_calculator/detail/TemplateHelpersFFAvg.h>
// #include <form_factor/FormFactorType.h>
// #include <data/Molecule.h>
// #include <data/record/Atom.h>
// #include <data/record/Water.h>
// #include <settings/HistogramSettings.h>
// #include <settings/GeneralSettings.h>
// #include <constants/Constants.h>
// #include <utility/MultiThreading.h>

// using namespace container;
// using namespace hist;

// template<bool use_weighted_distribution>
// HistogramManagerMTFFAvg<use_weighted_distribution>::~HistogramManagerMTFFAvg() = default;

// template<bool use_weighted_distribution>
// std::unique_ptr<DistanceHistogram> HistogramManagerMTFFAvg<use_weighted_distribution>::calculate() {return calculate_all();}

// template<bool use_weighted_distribution>
// std::unique_ptr<ICompositeDistanceHistogram> HistogramManagerMTFFAvg<use_weighted_distribution>::calculate_all() {
//     using GenericDistribution1D_t = typename hist::GenericDistribution1D<use_weighted_distribution>::type;
//     using GenericDistribution2D_t = typename hist::GenericDistribution2D<use_weighted_distribution>::type;
//     using GenericDistribution3D_t = typename hist::GenericDistribution3D<use_weighted_distribution>::type;
//     auto pool = utility::multi_threading::get_global_pool();

//     data_a_ptr = std::make_unique<hist::detail::CompactCoordinatesFF>(this->protein->get_bodies());
//     data_w_ptr = std::make_unique<hist::detail::CompactCoordinatesFF>(this->protein->get_waters());
//     auto& data_a = *data_a_ptr;
//     auto& data_w = *data_w_ptr;
//     int data_a_size = (int) data_a.size();
//     int data_w_size = (int) data_w.size();

//     //########################//
//     // PREPARE MULTITHREADING //
//     //########################//
//     auto calc_aa = [&data_a, data_a_size] (int imin, int imax) {
//         GenericDistribution3D_t p_aa(form_factor::get_count(), form_factor::get_count(), constants::axes::d_axis.bins, 0); // ff_type1, ff_type2, distance
//         for (int i = imin; i < imax; ++i) { // atom
//             int j = i+1;                    // atom
//             for (; j+7 < data_a_size; j+=8) {
//                 evaluate8<use_weighted_distribution, 2>(p_aa, data_a, data_a, i, j);
//             }

//             for (; j+3 < data_a_size; j+=4) {
//                 evaluate4<use_weighted_distribution, 2>(p_aa, data_a, data_a, i, j);
//             }

//             for (; j < data_a_size; ++j) {
//                 evaluate1<use_weighted_distribution, 2>(p_aa, data_a, data_a, i, j);
//             }
//         }
//         return p_aa;
//     };

//     auto calc_aw = [&data_w, &data_a, data_w_size] (int imin, int imax) {
//         GenericDistribution2D_t p_aw(form_factor::get_count(), constants::axes::d_axis.bins, 0); // ff_type, distance
//         for (int i = imin; i < imax; ++i) { // atom
//             int j = 0;                      // water
//             for (; j+7 < data_w_size; j+=8) {
//                 evaluate8<use_weighted_distribution, 1>(p_aw, data_a, data_w, i, j);
//             }

//             for (; j+3 < data_w_size; j+=4) {
//                 evaluate4<use_weighted_distribution, 1>(p_aw, data_a, data_w, i, j);
//             }

//             for (; j < data_w_size; ++j) {
//                 evaluate1<use_weighted_distribution, 1>(p_aw, data_a, data_w, i, j);
//             }
//         }
//         return p_aw;
//     };

//     auto calc_ww = [&data_w, data_w_size] (int imin, int imax) {
//         GenericDistribution1D_t p_ww(constants::axes::d_axis.bins, 0);
//         for (int i = imin; i < imax; ++i) { // water
//             int j = i+1;                    // water
//             for (; j+7 < data_w_size; j+=8) {
//                 evaluate8<use_weighted_distribution, 2>(p_ww, data_w, data_w, i, j);
//             }

//             for (; j+3 < data_w_size; j+=4) {
//                 evaluate4<use_weighted_distribution, 2>(p_ww, data_w, data_w, i, j);
//             }

//             for (; j < data_w_size; ++j) {
//                 evaluate1<use_weighted_distribution, 2>(p_ww, data_w, data_w, i, j);
//             }
//         }
//         return p_ww;
//     };

//     //##############//
//     // SUBMIT TASKS //
//     //##############//
//     int job_size = settings::general::detail::job_size;
//     BS::multi_future<GenericDistribution3D_t> aa;
//     for (int i = 0; i < (int) data_a_size; i+=job_size) {
//         aa.push_back(pool->submit(calc_aa, i, std::min<int>(i+job_size, (int) data_a_size)));
//     }
//     BS::multi_future<GenericDistribution2D_t> aw;
//     for (int i = 0; i < (int) data_a_size; i+=job_size) {
//         aw.push_back(pool->submit(calc_aw, i, std::min<int>(i+job_size, (int) data_a_size)));
//     }
//     BS::multi_future<GenericDistribution1D_t> ww;
//     for (int i = 0; i < (int) data_w_size; i+=job_size) {
//         ww.push_back(pool->submit(calc_ww, i, std::min<int>(i+job_size, (int) data_w_size)));
//     }

//     //#################//
//     // COLLECT RESULTS //
//     //#################//
//     auto p_aa_future = pool->submit(
//         [&]() {
//             GenericDistribution3D_t p_aa(form_factor::get_count(), form_factor::get_count(), constants::axes::d_axis.bins, 0); // ff_type1, ff_type2, distance
//             for (const auto& tmp : aa.get()) {
//                 std::transform(p_aa.begin(), p_aa.end(), tmp.begin(), p_aa.begin(), std::plus<>());
//             }
//             return p_aa;
//         }
//     );

//     auto p_aw_future = pool->submit(
//         [&]() {
//             GenericDistribution2D_t p_aw(form_factor::get_count(), constants::axes::d_axis.bins, 0); // ff_type, distance
//             for (const auto& tmp : aw.get()) {
//                 std::transform(p_aw.begin(), p_aw.end(), tmp.begin(), p_aw.begin(), std::plus<>());
//             }
//             return p_aw;
//         }
//     );

//     auto p_ww_future = pool->submit(
//         [&]() {
//             GenericDistribution1D_t p_ww(constants::axes::d_axis.bins, 0); // distance
//             for (const auto& tmp : ww.get()) {
//                 std::transform(p_ww.begin(), p_ww.end(), tmp.begin(), p_ww.begin(), std::plus<>());
//             }
//             return p_ww;
//         }
//     );
//     pool->wait_for_tasks();
//     auto p_aa = p_aa_future.get();
//     auto p_aw = p_aw_future.get();
//     auto p_ww = p_ww_future.get();

//     //###################//
//     // SELF-CORRELATIONS //
//     //###################//
//     for (int i = 0; i < data_a_size; ++i) {p_aa.index(data_a.get_ff_type(i), data_a.get_ff_type(i), 0) += std::pow(data_a[i].value.w, 2);}
//     p_aa.index(form_factor::exv_bin, form_factor::exv_bin, 0) = data_a_size;
//     p_ww.index(0) = std::accumulate(data_w.get_data().begin(), data_w.get_data().end(), 0.0, [](double sum, const hist::detail::CompactCoordinatesData& data) {return sum + std::pow(data.value.w, 2);});

//     // this is counter-intuitive, but splitting the loop into separate parts is likely faster since it allows both SIMD optimizations and better cache usage
//     Distribution1D p_tot(constants::axes::d_axis.bins, 0);
//     {   // sum all elements to the total
//         for (unsigned int ff1 = 0; ff1 < form_factor::get_count_without_excluded_volume(); ++ff1) {
//             for (unsigned int ff2 = 0; ff2 < form_factor::get_count_without_excluded_volume(); ++ff2) {
//                 std::transform(p_tot.begin(), p_tot.end(), p_aa.begin(ff1, ff2), p_tot.begin(), std::plus<>());
//             }
//         }
//         for (unsigned int ff1 = 0; ff1 < form_factor::get_count_without_excluded_volume(); ++ff1) {
//             std::transform(p_tot.begin(), p_tot.end(), p_aw.begin(ff1), p_tot.begin(), std::plus<>());
//         }
//         std::transform(p_tot.begin(), p_tot.end(), p_ww.begin(), p_tot.begin(), std::plus<>());
//     }

//     // downsize our axes to only the relevant area
//     unsigned int max_bin = 10; // minimum size is 10
//     for (unsigned int i = p_tot.size()-1; i >= 10; --i) {
//         if (p_tot.index(i) != 0) {
//             max_bin = i+1; // +1 since we usually use this for looping (i.e. i < max_bin)
//             break;
//         }
//     }
//     p_aa.resize(max_bin);
//     p_aw.resize(max_bin);
//     p_ww.resize(max_bin);

//     // multiply the excluded volume charge onto the excluded volume bins
//     double Z_exv_avg = this->protein->get_volume_grid()*constants::charge::density::water/this->protein->atom_size();
//     for (unsigned int ff1 = 0; ff1 < form_factor::get_count_without_excluded_volume(); ++ff1) {
//         std::transform(p_aa.begin(ff1, form_factor::exv_bin), p_aa.end(ff1, form_factor::exv_bin), p_aa.begin(ff1, form_factor::exv_bin), [Z_exv_avg] (auto val) {return val*Z_exv_avg;});
//     }
//     std::transform(p_aa.begin(form_factor::exv_bin, form_factor::exv_bin), p_aa.end(form_factor::exv_bin, form_factor::exv_bin), p_aa.begin(form_factor::exv_bin, form_factor::exv_bin), [Z_exv_avg] (auto val) {return val*Z_exv_avg*Z_exv_avg;});
//     std::transform(p_aw.begin(form_factor::exv_bin), p_aw.end(form_factor::exv_bin), p_aw.begin(form_factor::exv_bin), [Z_exv_avg] (auto val) {return val*Z_exv_avg;});
//     return std::make_unique<CompositeDistanceHistogramFFAvg>(
//         std::move(p_aa), 
//         std::move(p_aw), 
//         std::move(p_ww), 
//         Axis(0, max_bin*constants::axes::d_axis.width(), max_bin)
//     );
// }

// template class hist::HistogramManagerMTFFAvg<false>;
// template class hist::HistogramManagerMTFFAvg<true>;