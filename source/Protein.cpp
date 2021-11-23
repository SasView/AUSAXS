// includes
#include <vector>
#include <map>
#include "boost/format.hpp"
#include <utility>

// ROOT
#include <TVector3.h>

// my own includes
#include "data/Atom.h"
#include "hydrate/Grid.h"
#include "data/PDB_file.cpp"
#include "data/properties.h"
#include "Protein.h"
#include "settings.h"

using boost::format;
using std::vector, std::string, std::cout, std::endl, std::unique_ptr;
using namespace ROOT;

Protein::Protein(string path) {
    // determine which kind of input file we're looking at
    if (path.find(".xml") != string::npos) { // .xml file
        print_err("Error in Protein::Protein: .xml input files are not supported.");
    } else if (path.find(".pdb") != string::npos) { // .pdb file
        file = std::make_shared<PDB_file>(path);
    } else { // anything else - we cannot handle this
        print_err((format("Error in Protein::Protein: Invalid file extension of input file %1%.") % path).str());
        exit(1);
    }
    
    std::tie(protein_atoms, hydration_atoms) = file->get_atoms();
}

void Protein::save(string path) const {
    file->update(protein_atoms, hydration_atoms); // update the File backing this Protein with our new atoms
    file->write(path); // write to disk
}

void Protein::calc_distances() {
    // calculate the internal distances for the protein atoms
    int n_pp = 0; // index counter
    int n_hh = 0;
    int n_hp = 0;
    vector<double> d_pp(pow(protein_atoms.size(), 2));
    vector<double> w_pp(pow(protein_atoms.size(), 2)); 
    vector<double> d_hh(pow(hydration_atoms.size(), 2)); 
    vector<double> w_hh(pow(hydration_atoms.size(), 2)); 
    vector<double> d_hp(hydration_atoms.size()*protein_atoms.size());
    vector<double> w_hp(hydration_atoms.size()*protein_atoms.size()); 

    // calculate p-p distances
    for (int i = 0; i < protein_atoms.size(); i++) {
        for (int j = 0; j < protein_atoms.size(); j++) {
            d_pp[n_pp] = protein_atoms[i]->distance(protein_atoms[j]);
            w_pp[n_pp] = property::charge::get.at(protein_atoms[i]->get_element())*property::charge::get.at(protein_atoms[j]->get_element())
                *protein_atoms[i]->get_occupancy()*protein_atoms[j]->get_occupancy(); // Z1*Z2*w1*w2
            n_pp++;
        }
    }

    for (int i = 0; i < hydration_atoms.size(); i++) {
        // calculate h-h distances
        for (int j = 0; j < hydration_atoms.size(); j++) {
            d_hh[n_hh] = hydration_atoms[i]->distance(hydration_atoms[j]);
            w_hh[n_hh] = property::charge::get.at(hydration_atoms[i]->get_element())*property::charge::get.at(hydration_atoms[j]->get_element())
                *hydration_atoms[i]->get_occupancy()*hydration_atoms[j]->get_occupancy(); // Z1*Z2*w1*w2
            n_hh++;
        }
        // calculate h-p distances
        for (int j = 0; j < protein_atoms.size(); j++) {
            d_hp[n_hp] = hydration_atoms[i]->distance(protein_atoms[j]);
            w_hp[n_hp] = property::charge::get.at(hydration_atoms[i]->get_element())*property::charge::get.at(protein_atoms[j]->get_element())
                *hydration_atoms[i]->get_occupancy()*protein_atoms[j]->get_occupancy(); // Z1*Z2*w1*w2
            n_hp++;
        }
    }
    this->distances = std::make_shared<Distances>(d_pp, d_hh, d_hp, w_pp, w_hh, w_hp);
}

void Protein::generate_new_hydration() {
    // delete the old hydration layer
    hydration_atoms = vector<shared_ptr<Hetatom>>();

    // move protein to center of mass
    TVector3 cm = get_cm();
    translate(-cm);

    grid = std::make_shared<Grid>(TVector3(-250, -250, -250), setting::protein::grid_width, 501/setting::protein::grid_width); 
    grid->add(protein_atoms);
    hydration_atoms = grid->hydrate();
}

// vector<double> Protein::debye_scattering_intensity() const {
//     Distances distances = calc_distances();

//     // p is the bin contents of a histogram of the distances
//     vector<int> axes = {600, 0, 60};
//     double width = (double) (axes[2]-axes[1])/axes[0];
//     vector<double> p(axes[0]);
//     for (int i = 0; i < distances.pp.size(); i++) {
//         p[std::round(distances.pp[i]/width)] += distances.wpp[i];
//     }
//     for (int i = 0; i < distances.hh.size(); i++) {
//         p[std::round(distances.hh[i]/width)] += distances.whh[i];
//     }
//     for (int i = 0; i < distances.hp.size(); i++) {
//         p[std::round(distances.hp[i]/width)] += distances.whp[i];
//     }
//     return debye_scattering_intensity(axes, p);
// }

void Protein::generate_volume_file(string path) {
    vector<vector<vector<char>>>& g = grid->grid;
    vector<shared_ptr<Atom>> filled;
    for (int i = 0; i < g.size(); i++) {
        for (int j = 0; j < g[0].size(); j++) {
            for (int k = 0; k < g[0][0].size(); k++) {
                if (g[i][j][k] != 0) {
                    shared_ptr<Atom> a = std::make_shared<Atom>(0, "C", "", "C", "", 1, "", TVector3(i, j, k), 1, 0, "C", "");
                    filled.push_back(a);
                }
            }
        }
    }
    protein_atoms = filled;
    hydration_atoms = vector<shared_ptr<Hetatom>>();
    save(path);
    exit(0);
}

// vector<double> Protein::debye_scattering_intensity(vector<int> p_axes, vector<double>& p) const {
//     // calculate the Debye scattering intensity
//     const vector<double>& debye_axes = setting::protein::debye_scattering_plot_axes;
//     vector<double> Iq(debye_axes[0], 0);

//     vector<double> d(p_axes[0], 0);
//     double p_width = (double) (p_axes[2]-p_axes[1])/p_axes[0];
//     for (int i = 0; i < p_axes[0]; i++) {
//         d[i] = p_axes[1] + p_width*i;
//     }

//     double debye_width = (double) (debye_axes[2]-debye_axes[1])/debye_axes[0];
//     for (int i = 0; i < debye_axes[0]; i++) {
//         double q = debye_axes[1] + i*debye_width;
//         // cout << "Bin " << i << ", q: " << q << endl;
//         for (int j = 0; j < p_axes[0]; j++) {
//             if (q*d[j] < 1e-9) {
//                 Iq[i] += p[j];
//             } else {
//                 Iq[i] += p[j]*sin(q*d[j])/(q*d[j]);
//             }
//         }
//         // cout << endl << endl;;
//     }
//     return Iq;
// }

TVector3 Protein::get_cm() const {
    TVector3 cm;
    double M = 0; // total mass
    auto weighted_sum = [&cm, &M] (auto atoms) {
        for (auto const& a : *atoms) {
            double m = a->get_atomic_weight();
            M += m;
            double x = a->get_x()*m;
            double y = a->get_y()*m;
            double z = a->get_z()*m;
            cm += TVector3(x, y, z);
        }
        cm[0] = cm[0]/M;
        cm[1] = cm[1]/M;
        cm[2] = cm[2]/M;
    };
    weighted_sum(&protein_atoms);
    weighted_sum(&hydration_atoms);
    return cm;
}

double Protein::get_volume() const {
    double v = 0;
    int cur_seq = 0; // sequence number of current acid
    for (auto const& a : protein_atoms) {
        int a_seq = a->get_resSeq(); // sequence number of current atom
        if (cur_seq != a_seq) { // check if we are still dealing with the same acid
            cur_seq = a_seq; // if not, update our current sequence number
            v += property::volume::get.at(a->get_resName()); // and add its volume to the running total
        }
    }
    return v;
}

shared_ptr<Distances> Protein::get_distances() {
    if (distances == nullptr) {
        calc_distances();
    }
    return distances;
}

void Protein::translate(const TVector3 v) {
    auto move = [&v] (auto atoms) {
        for (auto const& a : *atoms) {
            a->translate(v);
        }
    };
    move(&protein_atoms);
    move(&hydration_atoms);
}