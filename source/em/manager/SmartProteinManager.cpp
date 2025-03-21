/*
This software is distributed under the GNU Lesser General Public License v3.0. 
For more information, please refer to the LICENSE file in the project root.
*/

#include <em/manager/SmartProteinManager.h>
#include <em/detail/ImageStackBase.h>
#include <hist/intensity_calculator/CompositeDistanceHistogram.h>
#include <data/Molecule.h>
#include <data/Body.h>
#include <utility/Console.h>
#include <settings/HistogramSettings.h>
#include <settings/EMSettings.h>

#include <vector>
#include <cassert>
#include <functional>

using namespace ausaxs;
using namespace ausaxs::em::managers;
using namespace ausaxs::data;

SmartProteinManager::~SmartProteinManager() = default;

std::unique_ptr<hist::ICompositeDistanceHistogram> SmartProteinManager::get_histogram(double cutoff) {
    update_protein(cutoff);
    return protein->get_histogram();
}

std::vector<EMAtom> SmartProteinManager::generate_atoms(double cutoff) const {
    // we use a list since we will have to append quite a few other lists to it
    std::list<EMAtom> atoms;
    const auto& imagestack = images->images();
    unsigned int step = settings::em::sample_frequency;
    for (unsigned int i = 0; i < imagestack.size(); i += step) {
        std::list<EMAtom> im_atoms = imagestack[i].generate_atoms(cutoff);
        atoms.splice(atoms.end(), im_atoms); // move im_atoms to end of atoms
    }

    // convert list to vector
    return std::vector<EMAtom>(std::make_move_iterator(std::begin(atoms)), std::make_move_iterator(std::end(atoms)));
}

std::unique_ptr<data::Molecule> SmartProteinManager::generate_protein(double cutoff) const {
    std::vector<EMAtom> atoms = generate_atoms(cutoff);
    std::vector<Body> bodies(charge_levels.size());
    std::vector<Atom> current_atoms(atoms.size());

    if (atoms.empty()) {
        console::print_warning("Warning in SmartProteinManager::generate_protein: No voxels found for cutoff \"" + std::to_string(cutoff) + "\".");
        return std::make_unique<data::Molecule>(bodies);
    }

    if (charge_levels.empty()) {
        throw except::out_of_bounds("SmartProteinManager::generate_protein: charge_levels is empty.");
    }

    // sort vector so we can slice it into levels of charge density
    std::function<bool(double, double)> compare_func = [] (double v1, double v2) {return v1 < v2;};
    std::sort(atoms.begin(), atoms.end(), [&compare_func] (const EMAtom& atom1, const EMAtom& atom2) {return compare_func(atom1.charge_density(), atom2.charge_density());});

    unsigned int charge_index = 0, atom_index = 0, current_index = 0;
    double charge = charge_levels[charge_index]; // initialize charge

    while (compare_func(atoms[atom_index].charge_density(), cutoff)) {++atom_index;} // search for first atom with charge larger than the cutoff
    while (compare_func(charge, cutoff)) {charge = charge_levels[++charge_index];} // search for first charge level larger than the cutoff 
    while (atom_index < atoms.size()) {
        if (compare_func(atoms[atom_index].charge_density(), charge)) {
            current_atoms[current_index++] = atoms[atom_index++].get_atom();
        } else {
            // create the body for this charge bin
            current_atoms.resize(current_index);
            bodies[charge_index] = Body(current_atoms);

            // prepare the next body
            current_index = 0;
            current_atoms.resize(atoms.size());

            // increment the charge level
            if (charge_index+1 == charge_levels.size()) [[unlikely]] {
                throw except::unexpected("SmartProteinManager::generate_protein: Reached end of charge levels list.");
            }
            charge = charge_levels[++charge_index];
        }
    }
    
    // create the final body of the loop
    current_atoms.resize(current_index);
    bodies[charge_index] = Body(current_atoms);

    return std::make_unique<data::Molecule>(std::move(bodies));
}

void SmartProteinManager::toggle_histogram_manager_init(bool state) {
    static settings::hist::HistogramManagerChoice previous = settings::hist::histogram_manager;
    if (state) {
        settings::hist::histogram_manager = previous;
    } else {
        previous = settings::hist::histogram_manager;
        settings::hist::histogram_manager = settings::hist::HistogramManagerChoice::None;
    }
}

void SmartProteinManager::update_protein(double cutoff) {
    if (protein == nullptr || protein->size_atom() == 0) {
        toggle_histogram_manager_init(true);
        protein = generate_protein(cutoff); 
        protein->bind_body_signallers();
        previous_cutoff = cutoff;
        toggle_histogram_manager_init(false);
        return;
    }

    if (cutoff == previous_cutoff) {
        return;
    }

    // sanity check
    if (charge_levels.empty()) {
        throw except::unexpected("SmartProteinManager::update_protein: charge_levels is empty.");
    }

    std::unique_ptr<data::Molecule> new_protein = generate_protein(cutoff);
    // std::cout << "Found " << new_protein->atom_size() << " voxels with a cutoff larger than " << cutoff << std::endl;

    std::function<bool(double, double)> compare_positive = [] (double v1, double v2) {return v1 < v2;};
    std::function<bool(double, double)> compare_negative = [] (double v1, double v2) {return v1 > v2;};
    std::function<bool(double, double)> compare_func = 0 <= cutoff ? compare_positive : compare_negative;

    if (compare_func(cutoff, previous_cutoff)) {
        // since cutoff is smaller than previously, we have to change all bins in the range [cutoff, previous_cutoff]

        // skip all bins before the relevant range
        unsigned int charge_index = 0;
        double current_cutoff = charge_levels[0];
        while (compare_func(current_cutoff, cutoff) && charge_index < charge_levels.size()) {
            current_cutoff = charge_levels[++charge_index];
        }

        // iterate through the remaining bins, and use a break statement to stop when we leave the relevant range
        for (; charge_index < charge_levels.size(); ++charge_index) {
            // check if the current bin is inside the range
            if (compare_func(charge_levels[charge_index], previous_cutoff)) {
                // if so, we replace it with the new contents
                protein->get_body(charge_index) = std::move(new_protein->get_body(charge_index));
            } else {
                // if we have the same number of atoms as earlier, nothing has changed
                if (new_protein->get_body(charge_index).size_atom() == protein->get_body(charge_index).size_atom()) {
                    break;
                }

                // otherwise we replace it and stop iterating
                protein->get_body(charge_index) = std::move(new_protein->get_body(charge_index));
                break;
            }
        }
    } else {
        // since cutoff is larger than previously, we have to change all bins in the range [previous_cutoff, cutoff]

        // skip all bins before the relevant range
        unsigned int charge_index = 0;
        double current_cutoff = charge_levels[0];
        while (compare_func(current_cutoff, previous_cutoff) && charge_index < charge_levels.size()) {
            current_cutoff = charge_levels[++charge_index];
        }

        // iterate through the remaining bins, and use a break statement to stop when we leave the relevant range
        for (; charge_index < charge_levels.size(); ++charge_index) {
            // check if the current bin is inside the range
            if (compare_func(charge_levels[charge_index], cutoff)) {
                // if so, we replace it with the new contents
                protein->get_body(charge_index) = std::move(new_protein->get_body(charge_index));
            } else {
                // otherwise we replace it and stop iterating
                protein->get_body(charge_index) = std::move(new_protein->get_body(charge_index));
                break;
            }
        }

    }

    previous_cutoff = cutoff;
}

observer_ptr<const data::Molecule> SmartProteinManager::get_protein() const {
    assert(protein != nullptr && "SmartProteinManager::get_protein: Protein has not been initialized yet.");
    return protein.get();
}

observer_ptr<data::Molecule> SmartProteinManager::get_protein(double cutoff) {
    update_protein(cutoff);
    return protein.get();
}

void SmartProteinManager::set_charge_levels(const std::vector<double>& levels) noexcept {
    ProteinManager::set_charge_levels(levels);
    protein = nullptr; // the protein must be generated anew to ensure the bodies remains in sync with the new levels
}