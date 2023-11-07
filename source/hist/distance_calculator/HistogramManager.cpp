#include <data/record/Atom.h>
#include <data/record/Water.h>
#include <data/Body.h>
#include <data/Molecule.h>
#include <data/state/StateManager.h>
#include <hist/distance_calculator/HistogramManager.h>
#include <hist/intensity_calculator/DistanceHistogram.h>
#include <hist/intensity_calculator/CompositeDistanceHistogram.h>
#include <hist/detail/CompactCoordinates.h>
#include <hist/distribution/GenericDistribution1D.h>
#include <hist/distribution/WeightedDistribution.h>
#include <settings/HistogramSettings.h>
#include <constants/Constants.h>
#include <hist/distance_calculator/detail/TemplateHelpers.h>

using namespace hist;

template<bool use_weighted_distribution>
HistogramManager<use_weighted_distribution>::HistogramManager(view_ptr<const data::Molecule> protein) : IHistogramManager(protein), protein(protein) {
    initialize();
}

template<bool use_weighted_distribution>
HistogramManager<use_weighted_distribution>::~HistogramManager() = default;

template<bool use_weighted_distribution>
std::unique_ptr<DistanceHistogram> HistogramManager<use_weighted_distribution>::calculate() {return calculate_all();}

template<>
void HistogramManager<true>::initialize() const {hist::WeightedDistribution::reset();}

template<>
void HistogramManager<false>::initialize() const {}

template<bool use_weighted_distribution>
std::unique_ptr<ICompositeDistanceHistogram> HistogramManager<use_weighted_distribution>::calculate_all() {
    using GenericDistribution1D_t = typename hist::GenericDistribution1D<use_weighted_distribution>::type;

    GenericDistribution1D_t p_pp(constants::axes::d_axis.bins, 0);
    GenericDistribution1D_t p_hh(constants::axes::d_axis.bins, 0);
    GenericDistribution1D_t p_hp(constants::axes::d_axis.bins, 0);

    hist::detail::CompactCoordinates data_p(protein->get_bodies());
    hist::detail::CompactCoordinates data_h = hist::detail::CompactCoordinates(protein->get_waters());

    // calculate p-p distances
    for (unsigned int i = 0; i < data_p.get_size(); ++i) {
        unsigned int j = i+1;
        for (; j+7 < data_p.get_size(); j+=8) {
            evaluate8<use_weighted_distribution, 2>(p_pp, data_p, data_p, i, j);
        }

        for (; j+3 < data_p.get_size(); j+=4) {
            evaluate4<use_weighted_distribution, 2>(p_pp, data_p, data_p, i, j);
        }

        for (; j < data_p.get_size(); ++j) {
            evaluate1<use_weighted_distribution, 2>(p_pp, data_p, data_p, i, j);
        }
    }

    for (unsigned int i = 0; i < data_h.get_size(); ++i) {
        // calculate h-h distances
        {
            unsigned int j = i+1;
            for (; j+7 < data_h.get_size(); j+=8) {
                evaluate8<use_weighted_distribution, 2>(p_hh, data_h, data_h, i, j);
            }

            for (; j+3 < data_h.get_size(); j+=4) {
                evaluate4<use_weighted_distribution, 2>(p_hh, data_h, data_h, i, j);
            }

            for (; j < data_h.get_size(); ++j) {
                evaluate1<use_weighted_distribution, 2>(p_hh, data_h, data_h, i, j);
            }
        }
        
        // calculate h-p distances
        {
            unsigned int j = 0;
            for (; j+7 < data_p.get_size(); j+=8) {
                evaluate8<use_weighted_distribution, 1>(p_hp, data_h, data_p, i, j);
            }

            for (; j+3 < data_p.get_size(); j+=4) {
                evaluate4<use_weighted_distribution, 1>(p_hp, data_h, data_p, i, j);
            }

            for (; j < data_p.get_size(); ++j) {
                evaluate1<use_weighted_distribution, 1>(p_hp, data_h, data_p, i, j);
            }
        }
    }

    // add self-correlation
    p_pp.add(0, std::accumulate(data_p.get_data().begin(), data_p.get_data().end(), 0.0, [](double sum, const hist::detail::CompactCoordinatesData& val) {return sum + std::pow(val.value.w, 2);}));
    p_hh.add(0, std::accumulate(data_h.get_data().begin(), data_h.get_data().end(), 0.0, [](double sum, const hist::detail::CompactCoordinatesData& val) {return sum + std::pow(val.value.w, 2);}));

    // calculate p_tot
    Distribution1D p_tot(constants::axes::d_axis.bins, 0);
    for (unsigned int i = 0; i < p_pp.size(); ++i) {p_tot.index(i) = p_pp.index(i) + p_hh.index(i) + 2*p_hp.index(i);}

    // downsize our axes to only the relevant area
    unsigned int max_bin = 10; // minimum size is 10
    for (int i = p_tot.size()-1; i >= 10; i--) {
        if (p_tot.index(i) != 0) {
            max_bin = i+1; // +1 since we usually use this for looping (i.e. i < max_bin)
            break;
        }
    }

    p_pp.resize(max_bin);
    p_hh.resize(max_bin);
    p_hp.resize(max_bin);
    p_tot.resize(max_bin);
    return std::make_unique<CompositeDistanceHistogram>(
        std::move(p_pp), 
        std::move(p_hp), 
        std::move(p_hh), 
        std::move(p_tot), 
        Axis(0, max_bin*constants::axes::d_axis.width(), max_bin)
    );
}

template class hist::HistogramManager<false>;
template class hist::HistogramManager<true>;