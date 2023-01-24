#include <em/SimpleProteinManager.h>
#include <hist/HistogramManagerMT.h>
#include <data/Protein.h>
#include <memory>

void em::SimpleProteinManager::update_protein(double cutoff) {
    protein = std::make_shared<Protein>(generate_atoms(cutoff));
    protein->set_histogram_manager<hist::HistogramManagerMT>();
}

void em::SimpleProteinManager::set_charge_levels(std::vector<double>) noexcept {}

void em::SimpleProteinManager::set_charge_levels(Axis) noexcept {}