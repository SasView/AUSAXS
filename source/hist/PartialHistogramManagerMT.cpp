#include <hist/PartialHistogramManagerMT.h>
#include <hist/DistanceHistogram.h>
#include <hist/CompositeDistanceHistogram.h>
#include <data/Protein.h>
#include <data/Body.h>
#include <data/Atom.h> 
#include <data/Water.h>
#include <settings/GeneralSettings.h>
#include <settings/HistogramSettings.h>
#include <data/state/StateManager.h>

#include <mutex>
#include <list>
#include <BS_thread_pool.hpp>

using namespace hist;

PartialHistogramManagerMT::PartialHistogramManagerMT(Protein* protein) : PartialHistogramManager(protein), pool(std::make_unique<BS::thread_pool>(settings::general::threads)) {}

PartialHistogramManagerMT::PartialHistogramManagerMT(PartialHistogramManager& phm) : PartialHistogramManager(phm), pool(std::make_unique<BS::thread_pool>(settings::general::threads)) {}

PartialHistogramManagerMT::~PartialHistogramManagerMT() = default;

std::unique_ptr<DistanceHistogram> PartialHistogramManagerMT::calculate() {
    std::vector<BS::multi_future<std::vector<double>>> futures_self_corr;
    std::vector<BS::multi_future<std::vector<double>>> futures_pp;
    std::vector<BS::multi_future<std::vector<double>>> futures_hp;
    BS::multi_future<std::vector<double>> futures_hh;
    futures_self_corr.reserve(body_size);
    futures_pp.reserve(body_size*(body_size-1)/2);
    futures_hp.reserve(body_size);

    const std::vector<bool>& externally_modified = statemanager->get_externally_modified_bodies();
    const std::vector<bool>& internally_modified = statemanager->get_internally_modified_bodies();
    const bool hydration_modified = statemanager->get_modified_hydration();

    // check if the object has already been initialized
    if (master.get_counts().size() == 0) [[unlikely]] {
        initialize(); 
    }

    // if not, we must first check if the atom coordinates have been changed in any of the bodies
    else {
        for (unsigned int i = 0; i < body_size; ++i) {

            // if the internal state was modified, we have to recalculate the self-correlation
            if (internally_modified[i]) {
                futures_self_corr.push_back(calc_self_correlation(i));
            }

            // if the external state was modified, we have to update the coordinate representations for later calculations
            else if (externally_modified[i]) {
                pool->push_task(&PartialHistogramManagerMT::update_compact_representation_body, this, i);
            }
        }

        // merge the partial results from each thread and add it to the master histogram
        unsigned int counter = 0;
        for (unsigned int i = 0; i < body_size; ++i) {
            if (internally_modified[i]) {
                pool->push_task(&PartialHistogramManagerMT::combine_self_correlation, this, i, std::ref(futures_self_corr[counter++]));
            }
        }
    }

    // small efficiency improvement: if the hydration layer was modified, we can update the compact representations in parallel with the self-correlation
    if (hydration_modified) {
        pool->push_task(&PartialHistogramManagerMT::update_compact_representation_water, this);
    }
    pool->wait_for_tasks(); // ensure the compact representations have been updated before continuing

    // check if the hydration layer was modified
    if (hydration_modified) {
        futures_hh = calc_hh();
    }

    // iterate through the lower triangle and check if either of each pair of bodies was modified
    for (unsigned int i = 0; i < body_size; ++i) {
        for (unsigned int j = 0; j < i; ++j) {
            if (externally_modified[i] || externally_modified[j]) {
                // one of the bodies was modified, so we recalculate its partial histogram
                futures_pp.push_back(calc_pp(i, j));
            }
        }

        // we also have to remember to update the partial histograms with the hydration layer
        if (externally_modified[i] || hydration_modified) {
            futures_hp.push_back(calc_hp(i));
        }
    }

    // merge the partial results from each thread and add it to the master histogram
    {
        if (hydration_modified) {
            pool->push_task(&PartialHistogramManagerMT::combine_hh, this, std::ref(futures_hh));
        }

        unsigned int counter_pp = 0, counter_hp = 0;
        for (unsigned int i = 0; i < body_size; ++i) {
            for (unsigned int j = 0; j < i; ++j) {
                if (externally_modified[i] || externally_modified[j]) {
                    pool->push_task(&PartialHistogramManagerMT::combine_pp, this, i, j, std::ref(futures_pp[counter_pp++]));
                }
            }

            if (externally_modified[i] || hydration_modified) {
                pool->push_task(&PartialHistogramManagerMT::combine_hp, this, i, std::ref(futures_hp[counter_hp++]));
            }
        }
    }

    statemanager->reset();
    pool->wait_for_tasks();
    std::vector<double> p = master.get_counts();
    return std::make_unique<DistanceHistogram>(std::move(p), master.get_axis());
}

void PartialHistogramManagerMT::update_compact_representation_body(unsigned int index) {
    coords_p[index] = detail::CompactCoordinates(protein->get_body(index));
}

void PartialHistogramManagerMT::update_compact_representation_water() {
    coords_h = detail::CompactCoordinates(protein->get_waters());
}

std::unique_ptr<CompositeDistanceHistogram> PartialHistogramManagerMT::calculate_all() {
    auto total = calculate();
    total->shorten_axis();
    unsigned int bins = total->get_axis().bins;

    // after calling calculate(), everything is already calculated, and we only have to extract the individual contributions
    std::vector<double> p_hh = partials_hh.get_counts();
    std::vector<double> p_pp = master.base.get_counts();
    std::vector<double> p_hp(bins, 0);
    // iterate through all partial histograms in the upper triangle
    for (unsigned int i = 0; i < body_size; ++i) {
        for (unsigned int j = 0; j <= i; ++j) {
            detail::PartialHistogram& current = partials_pp.index(i, j);

            // iterate through each entry in the partial histogram
            for (unsigned int k = 0; k < bins; ++k) {
                p_pp[k] += current.get_count(k); // add to p_pp
            }
        }
    }

    // iterate through all partial hydration-protein histograms
    for (unsigned int i = 0; i < body_size; ++i) {
        detail::PartialHistogram& current = partials_hp.index(i);

        // iterate through each entry in the partial histogram
        for (unsigned int k = 0; k < bins; ++k) {
            p_hp[k] += current.get_count(k); // add to p_pp
        }
    }

    // p_hp is already resized
    p_hh.resize(bins);
    p_pp.resize(bins);

    return std::make_unique<CompositeDistanceHistogram>(std::move(p_pp), std::move(p_hp), std::move(p_hh), std::move(total->get_counts()), total->get_axis());
}

/**
 * @brief This initializes some necessary variables and precalculates the internal distances between atoms in each body.
 */
void PartialHistogramManagerMT::initialize() {
    Axis axis(0, settings::axes::max_distance, settings::axes::max_distance/settings::axes::distance_bin_width); 
    std::vector<double> p_base(axis.bins, 0);
    master = detail::MasterHistogram(p_base, axis);

    static std::vector<BS::multi_future<std::vector<double>>> futures;
    futures = std::vector<BS::multi_future<std::vector<double>>>(body_size);
    partials_hh = detail::PartialHistogram(axis);
    for (unsigned int i = 0; i < body_size; ++i) {
        partials_hp.index(i) = detail::PartialHistogram(axis);
        partials_pp.index(i, i) = detail::PartialHistogram(axis);
        futures[i] = calc_self_correlation(i);

        for (unsigned int j = 0; j < i; ++j) {
            partials_pp.index(i, j) = detail::PartialHistogram(axis);
        }
    }

    for (unsigned int i = 0; i < body_size; ++i) {
        pool->push_task(&PartialHistogramManagerMT::combine_self_correlation, this, i, std::ref(futures[i]));
    }
}

BS::multi_future<std::vector<double>> PartialHistogramManagerMT::calc_self_correlation(unsigned int index) {
    update_compact_representation_body(index);

    // calculate internal distances between atoms
    static auto calc_internal = [] (const detail::CompactCoordinates& coords, unsigned int pp_size, unsigned int imin, unsigned int imax) {
        double width = settings::axes::distance_bin_width;

        std::vector<double> p_pp(pp_size, 0);
        for (unsigned int i = imin; i < imax; ++i) {
            for (unsigned int j = i+1; j < coords.size; ++j) {
                float weight = coords.data[i].w*coords.data[j].w;
                float dx = coords.data[i].x - coords.data[j].x;
                float dy = coords.data[i].y - coords.data[j].y;
                float dz = coords.data[i].z - coords.data[j].z;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                p_pp[dist/width] += 2*weight;
            }
        }
        return p_pp;
    };

    // calculate self correlation
    static auto calc_self = [] (const detail::CompactCoordinates& coords, unsigned int pp_size) {
        std::vector<double> p_pp(pp_size, 0);
        for (unsigned int i = 0; i < coords.size; ++i) {
            p_pp[0] += coords.data[i].w*coords.data[i].w;
        }
        return p_pp;
    };

    BS::multi_future<std::vector<double>> futures_self_corr;
    unsigned int atom_size = protein->atom_size();
    for (unsigned int i = 0; i < atom_size; i += settings::general::detail::job_size) {
        futures_self_corr.push_back(pool->submit(calc_internal, std::cref(coords_p[index]), master.get_axis().bins, i, std::min(i+settings::general::detail::job_size, atom_size)));
    }
    futures_self_corr.push_back(pool->submit(calc_self, std::cref(coords_p[index]), master.get_axis().bins));
    return futures_self_corr;
}

BS::multi_future<std::vector<double>> PartialHistogramManagerMT::calc_pp(unsigned int n, unsigned int m) {
    static auto calc_pp = [] (const detail::CompactCoordinates& coords_n, const detail::CompactCoordinates& coords_m, unsigned int pp_size, unsigned int imin, unsigned int imax) {
        double width = settings::axes::distance_bin_width;

        std::vector<double> p_pp(pp_size, 0);
        for (unsigned int i = imin; i < imax; ++i) {
            for (unsigned int j = 0; j < coords_m.size; ++j) {
                float weight = coords_n.data[i].w*coords_m.data[j].w;
                float dx = coords_n.data[i].x - coords_m.data[j].x;
                float dy = coords_n.data[i].y - coords_m.data[j].y;
                float dz = coords_n.data[i].z - coords_m.data[j].z;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                p_pp[dist/width] += 2*weight;
            }
        }
        return p_pp;
    };

    BS::multi_future<std::vector<double>> futures_pp;
    detail::CompactCoordinates& coords_n = coords_p[n];
    for (unsigned int i = 0; i < coords_n.size; i += settings::general::detail::job_size) {
        futures_pp.push_back(pool->submit(calc_pp, std::cref(coords_p[n]), std::cref(coords_p[m]), master.get_axis().bins, i, std::min(i+settings::general::detail::job_size, coords_n.size)));
    }
    return futures_pp;
}

BS::multi_future<std::vector<double>> PartialHistogramManagerMT::calc_hp(unsigned int index) {
    static auto calc_hp = [] (const detail::CompactCoordinates& coords_i, const detail::CompactCoordinates& coords_h, unsigned int hp_size, unsigned int imin, unsigned int imax) {
        double width = settings::axes::distance_bin_width;

        std::vector<double> p_hp(hp_size, 0);
        for (unsigned int i = imin; i < imax; ++i) {
            for (unsigned int j = 0; j < coords_h.size; ++j) {
                float weight = coords_i.data[i].w*coords_h.data[j].w;
                float dx = coords_i.data[i].x - coords_h.data[j].x;
                float dy = coords_i.data[i].y - coords_h.data[j].y;
                float dz = coords_i.data[i].z - coords_h.data[j].z;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                p_hp[dist/width] += 2*weight;
            }
        }
        return p_hp;
    };

    BS::multi_future<std::vector<double>> futures_hp;
    detail::CompactCoordinates& coords = coords_p[index];
    for (unsigned int i = 0; i < coords.size; i += settings::general::detail::job_size) {
        futures_hp.push_back(pool->submit(calc_hp, std::cref(coords_p[index]), std::cref(coords_h), master.get_axis().bins, i, std::min(i+settings::general::detail::job_size, coords.size)));
    }
    return futures_hp;
}

BS::multi_future<std::vector<double>> PartialHistogramManagerMT::calc_hh() {
    // calculate internal distances for the hydration layer
    static auto calc_hh = [] (const detail::CompactCoordinates& coords_h, unsigned int hh_size, unsigned int imin, unsigned int imax) {
        double width = settings::axes::distance_bin_width;

        std::vector<double> p_hh(hh_size, 0);
        for (unsigned int i = imin; i < imax; ++i) {
            for (unsigned int j = i+1; j < coords_h.size; ++j) {
                float weight = coords_h.data[i].w*coords_h.data[j].w;
                float dx = coords_h.data[i].x - coords_h.data[j].x;
                float dy = coords_h.data[i].y - coords_h.data[j].y;
                float dz = coords_h.data[i].z - coords_h.data[j].z;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                p_hh[dist/width] += 2*weight;
            }
        }
        return p_hh;
    };

    // calculate self correlation
    static auto calc_self = [] (const detail::CompactCoordinates& coords_h, unsigned int hh_size) {
        std::vector<double> p_hh(hh_size, 0);
        for (unsigned int i = 0; i < coords_h.size; ++i) {
            p_hh[0] += coords_h.data[i].w*coords_h.data[i].w;
        }
        return p_hh;
    };

    BS::multi_future<std::vector<double>> futures_hh;
    for (unsigned int i = 0; i < coords_h.size; i += settings::general::detail::job_size) {
        futures_hh.push_back(pool->submit(calc_hh, std::cref(coords_h), master.get_axis().bins, i, std::min(i+settings::general::detail::job_size, coords_h.size)));
    }
    futures_hh.push_back(pool->submit(calc_self, std::cref(coords_h), master.get_axis().bins));
    return futures_hh;
}

void PartialHistogramManagerMT::combine_self_correlation(unsigned int index, BS::multi_future<std::vector<double>>& futures) {
    std::vector<double> p_pp(master.get_axis().bins, 0);

    // iterate through all partial results. Each partial result is from a different thread calculation
    for (auto& tmp : futures.get()) {
        std::transform(p_pp.begin(), p_pp.end(), tmp.begin(), p_pp.begin(), std::plus<double>());
    }

    // update the master histogram
    master_hist_mutex.lock();
    master -= partials_pp.index(index, index);
    partials_pp.index(index, index).get_counts() = std::move(p_pp);
    master += partials_pp.index(index, index);
    master_hist_mutex.unlock();
}

void PartialHistogramManagerMT::combine_pp(unsigned int n, unsigned int m, BS::multi_future<std::vector<double>>& futures) {
    std::vector<double> p_pp(master.get_axis().bins, 0);

    // iterate through all partial results. Each partial result is from a different thread calculation
    for (auto& tmp : futures.get()) {
        std::transform(p_pp.begin(), p_pp.end(), tmp.begin(), p_pp.begin(), std::plus<double>());
    }

    // update the master histogram
    master_hist_mutex.lock();
    master -= partials_pp.index(n, m);
    partials_pp.index(n, m).get_counts() = std::move(p_pp);
    master += partials_pp.index(n, m);
    master_hist_mutex.unlock();
}

void PartialHistogramManagerMT::combine_hp(unsigned int index, BS::multi_future<std::vector<double>>& futures) {
    std::vector<double> p_hp(master.get_axis().bins, 0);

    // iterate through all partial results. Each partial result is from a different thread calculation
    for (auto& tmp : futures.get()) {
        std::transform(p_hp.begin(), p_hp.end(), tmp.begin(), p_hp.begin(), std::plus<double>());
    }

    // update the master histogram
    master_hist_mutex.lock();
    master -= partials_hp.index(index); // subtract the previous hydration histogram
    partials_hp.index(index).get_counts() = std::move(p_hp);
    master += partials_hp.index(index); // add the new hydration histogram
    master_hist_mutex.unlock();
}

void PartialHistogramManagerMT::combine_hh(BS::multi_future<std::vector<double>>& futures) {
    std::vector<double> p_hh(master.get_axis().bins, 0);

    // iterate through all partial results. Each partial result is from a different thread calculation
    for (auto& tmp : futures.get()) {
        std::transform(p_hh.begin(), p_hh.end(), tmp.begin(), p_hh.begin(), std::plus<double>());
    }

    // update the master histogram
    master_hist_mutex.lock();
    master -= partials_hh; // subtract the previous hydration histogram
    partials_hh.get_counts() = std::move(p_hh);
    master += partials_hh; // add the new hydration histogram
    master_hist_mutex.unlock();
}