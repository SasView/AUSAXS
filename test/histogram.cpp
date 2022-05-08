#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <utility/Utility.h>
#include <histogram/ScatteringHistogram.h>
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

    hist::ScatteringHistogram hist = protein.get_histogram();
    vector<double> p_pp = hist.p_pp;
    vector<double> p_hp = hist.p_hp;
    vector<double> p_hh = hist.p_hh;

    hist.apply_water_scaling_factor(2);
    for (size_t i = 0; i < p_pp.size(); i++) {
        REQUIRE_THAT(p_pp[i] + 2*p_hp[i] + 4*p_hh[i], Catch::Matchers::WithinRel(hist.p[i]));
    }

    hist.apply_water_scaling_factor(3);
    for (size_t i = 0; i < p_pp.size(); i++) {
        REQUIRE_THAT(p_pp[i] + 3*p_hp[i] + 9*p_hh[i], Catch::Matchers::WithinRel(hist.p[i]));
    }

    hist.reset_water_scaling_factor();
    for (size_t i = 0; i < p_pp.size(); i++) {
        REQUIRE_THAT(p_pp[i] + p_hp[i] + p_hh[i], Catch::Matchers::WithinRel(hist.p[i]));
    }
}

// TEST_CASE("utility", "[histogram]") {
//     string s = "   hello   ";
//     CHECK(remove_spaces(s) == "hello");

    // s = "hello   ";
    // CHECK(remove_spaces(s) == "hello");

    // s = "   hello";
    // CHECK(remove_spaces(s) == "hello");

    // s = "   k    ";
    // CHECK(remove_spaces(s) == "k");
// }