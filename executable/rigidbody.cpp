// includes
#include <vector>
#include <string>
#include <iostream>

#include "data/Body.h"
#include "data/Protein.h"
#include "fitter/IntensityFitter.h"
#include "plots/PlotIntensityFit.h"
#include "plots/PlotIntensityFitResiduals.h"
#include "rigidbody/RigidBody.h"
#include "data/BodySplitter.h"
#include "CLI11.hpp"

using std::cout, std::endl;

int main(int argc, char const *argv[]) { 
    CLI::App app{"Rigid-body optimization."};

    string input_structure, input_measurement, output, placement_strategy;
    app.add_option("input_s", input_structure, "Path to the structure file.")->required()->check(CLI::ExistingFile);
    app.add_option("input_m", input_measurement, "Path to the measuremed data.")->required()->check(CLI::ExistingFile);
    app.add_option("output", output, "Path to save the hydrated file at.")->required();
    app.add_option("--reduce,-r", setting::grid::percent_water, "The desired number of water molecules as a percentage of the number of atoms. Use 0 for no reduction.");
    app.add_option("--grid_width,-w", setting::grid::width, "The distance between each grid point in Ångström (default: 1). Lower widths increase the precision.");
    app.add_option("--bin_width", setting::axes::scattering_intensity_plot_binned_width, "Bin width for the distance histograms. Default: 1.");
    app.add_option("--placement_strategy", placement_strategy, "The placement strategy to use. Options: Radial, Axes, Jan.");
    app.add_option("--radius_a", setting::grid::ra, "Radius of the protein atoms.");
    app.add_option("--radius_h", setting::grid::rh, "Radius of the hydration atoms.");
    app.add_option("--qlow", setting::fit::q_low, "Lower limit on used q values from measurement file.");
    app.add_option("--qhigh", setting::fit::q_high, "Upper limit on used q values from measurement file.");
    app.add_flag("--center,!--no-center", setting::protein::center, "Decides whether the protein will be centered. Default: true.");
    app.add_flag("--effective-charge,!--no-effective-charge", setting::protein::use_effective_charge, "Decides whether the protein will be centered. Default: true.");
    CLI11_PARSE(app, argc, argv);

    // parse strategy
    if (placement_strategy == "Radial") {setting::grid::psc = setting::grid::PlacementStrategyChoice::RadialStrategy;}
    else if (placement_strategy == "Axes") {setting::grid::psc = setting::grid::PlacementStrategyChoice::AxesStrategy;}
    else if (placement_strategy == "Jan") {setting::grid::psc = setting::grid::PlacementStrategyChoice::JanStrategy;}

    vector<int> splits = {9, 99};
    Protein protein = BodySplitter::split("data/LAR1-2.pdb", splits);
    RigidBody body(protein);

    body.optimize(input_measurement);
    return 0;
}