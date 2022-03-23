#include <catch2/catch.hpp>

#include <ScatteringHistogram.h>
#include <data/Protein.h>
#include <data/Atom.h>
#include <data/Hetatom.h>
#include <plots/PlotIntensity.h>

TEST_CASE("check_scaling_factor", "[histogram]") {
    // the following just describes the eight corners of a cube centered at origo, with an additional atom at the very middle
    vector<Atom> b1 =   {Atom(Vector3(-1, -1, -1), 1, "C", "C", 1),  Atom(Vector3(-1, 1, -1), 1, "C", "C", 1)};
    vector<Atom> b2 =   {Atom(Vector3(1, -1, -1), 1, "C", "C", 1),   Atom(Vector3(1, 1, -1), 1, "C", "C", 1)};
    vector<Atom> b3 =   {Atom(Vector3(-1, -1, 1), 1, "C", "C", 1),   Atom(Vector3(-1, 1, 1), 1, "C", "C", 1)};
    vector<Hetatom> w = {Hetatom(Vector3(1, -1, 1), 1, "C", "C", 1), Hetatom(Vector3(1, 1, 1), 1, "C", "C", 1)};
    vector<vector<Atom>> a = {b1, b2, b3};
    Protein protein(a, w);

    ScatteringHistogram hist = protein.get_histogram();
    vector<double> p_pp = hist.p_pp;
    vector<double> p_hp = hist.p_hp;
    vector<double> p_hh = hist.p_hh;

    hist.apply_water_scaling_factor(2);
    for (size_t i = 0; i < p_pp.size(); i++) {
        REQUIRE(p_pp[i] + 2*p_hp[i] + 4*p_hh[i] == Approx(hist.p_tot[i]));
    }

    hist.apply_water_scaling_factor(3);
    for (size_t i = 0; i < p_pp.size(); i++) {
        REQUIRE(p_pp[i] + 3*p_hp[i] + 9*p_hh[i] == Approx(hist.p_tot[i]));
    }

    hist.reset_water_scaling_factor();
    for (size_t i = 0; i < p_pp.size(); i++) {
        REQUIRE(p_pp[i] + p_hp[i] + p_hh[i] == Approx(hist.p_tot[i]));
    }
}

//#####################//
//### DATASET TESTS ###//
//#####################//
TEST_CASE("dataset_works", "[histogram]") {
    vector<double> x = {1, 2, 3, 4, 5};
    vector<double> y = {10, 20, 30, 40, 50};
    Dataset data(x, y, "i", "j");

    vector<double> i = data.get("i");
    vector<double> j = data.get("j");
    CHECK(i == x);
    CHECK(j == y);

    data.reduce(2);
    CHECK(data.size() < x.size());
}

TEST_CASE("reduce", "[histogram],[files],[manual]") {
    Protein protein("data/2epe.pdb");
    auto h = protein.get_histogram();

    plots::PlotIntensity plot(h);
    plot.plot_intensity(h.calc_debye_scattering_intensity().reduce(20));
    plot.save("reduce_test.pdf");
}

TEST_CASE("limit", "[histogram]") {
    std::cout << "NOT IMPLEMENTED YET!" << std::endl;
    exit(1);
}