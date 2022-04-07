#include <iostream>

#include <em/ImageStack.h>
#include <plots/PlotImage.h>
#include <plots/PlotIntensity.h>
#include <plots/PlotDistance.h>
#include <Exceptions.h>
#include <settings.h>

using std::string;

int main(int argc, char const *argv[]) {
    string pdb_file = argv[1];

    setting::protein::use_effective_charge = false;
    setting::fit::q_high = 0.4;

    // plot pdb file
    Protein protein(pdb_file);
    SAXSDataset data = protein.get_histogram().calc_debye_scattering_intensity();
    data.reduce(setting::fit::N, true);
    data.limit(Limit(setting::fit::q_low, setting::fit::q_high));
    data.simulate_errors();
    plots::PlotIntensity plot(data);

    // prepare fit colors
    setting::em::max_atoms = 10000;
    gStyle->SetPalette(kSolar);
    auto cols = TColor::GetPalette();

    unsigned int color_step = (cols.GetSize()-1)/argc;
    for (int i = 2; i < argc; i++) {
        std::cout << "Now fitting " << argv[i] << "..." << std::endl;
        em::ImageStack image(argv[i]);
        auto fit = image.fit(protein.get_histogram());
        plot.plot_intensity(fit, cols.At(color_step*(i-1)));
    }

    plot.save("figures/stuff.pdf");
    return 0;
}