// includes
#include <vector>
#include <string>
#include <fstream>

#include "data/Body.h"
#include "data/Protein.h"
#include "hydrate/Grid.h"
#include "settings.h"
#include "catch2/catch.hpp"

#include <math/Vector3.h>
 
using namespace ROOT;

TEST_CASE("grid_generation", "[grid]") {
    Vector3 base(-10, -10, -10);
    int width = 1;
    int bins = 21;
    Grid grid(base, width, bins);

    vector<Atom> a = {Atom({0, 0, 0}, 0, "C", "", 0)};
    grid.add(a);
    vector<vector<vector<char>>> &g = grid.grid;

    // check that it was placed correctly in the grid
    REQUIRE(g[10][10][10] == 'A');
    REQUIRE(g[10][10][11] == 0);
    REQUIRE(g[10][11][11] == 0);
    REQUIRE(g[11][10][10] == 0);
    REQUIRE(g[9][8][14] == 0);
}

TEST_CASE("bounding_box", "[grid]") {
    Vector3 base(-10, -10, -10);
    int width = 1;
    int bins = 21;
    Grid grid(base, width, bins);

    SECTION("simple") {
        vector<Atom> a = {Atom({0, 0, 0}, 0, "C", "", 0)};
        grid.add(a);

        vector<vector<int>> box = grid.bounding_box();
        REQUIRE(box[0][0] == 10);
        REQUIRE(box[0][1] == 11);
        REQUIRE(box[1][0] == 10);
        REQUIRE(box[1][1] == 11);
        REQUIRE(box[2][0] == 10);
        REQUIRE(box[2][1] == 11);
    }

    SECTION("complex") {
        vector<Atom> a = {Atom({5, 0, -7}, 0, "C", "", 1), Atom({0, -5, 0}, 0, "C", "", 2)};
        grid.add(a);
        grid.expand_volume();

        vector<vector<int>> box = grid.bounding_box();
        REQUIRE(box[0][0] == 10);
        REQUIRE(box[0][1] == 16);
        REQUIRE(box[1][0] == 5);
        REQUIRE(box[1][1] == 11);
        REQUIRE(box[2][0] == 3);
        REQUIRE(box[2][1] == 11);
    }
}

TEST_CASE("volume_expansion", "[grid]") {
    Vector3 base(-10, -10, -10);
    int width = 1;
    int bins = 21;
    int radius = 3;
    Grid grid(base, width, bins, radius);

    vector<Atom> a = {Atom({0, 0, 0}, 0, "C", "", 0)};
    grid.add(a);
    vector<vector<vector<char>>> &g = grid.grid;
    grid.expand_volume();

    REQUIRE(g[10][10][10] == 'A'); // check that the center is still marked as capital 'A'

    // check that the x=13 plane looks like this: 
    // x x x
    // x o x
    // x x x
    CHECK(g[14][10][10] == 0); // check that it does not extend to the x=14 plane
    CHECK(g[13][10][10] == 'a');

    CHECK(g[13][9][9] == 0);
    CHECK(g[13][9][10] == 0);
    CHECK(g[13][9][11] == 0);

    CHECK(g[13][10][9] == 0);
    CHECK(g[13][11][11] == 0);

    CHECK(g[13][11][9] == 0);
    CHECK(g[13][11][10] == 0);
    CHECK(g[13][11][11] == 0);

    // repeat with the x=7 plane
    CHECK(g[6][10][10] == 0); // check that it does not extend to the x=6 plane
    CHECK(g[7][10][10] == 'a');

    CHECK(g[7][9][9] == 0);
    CHECK(g[7][9][10] == 0);
    CHECK(g[7][9][11] == 0);

    CHECK(g[7][10][9] == 0);
    CHECK(g[7][11][11] == 0);

    CHECK(g[7][11][9] == 0);
    CHECK(g[7][11][10] == 0);
    CHECK(g[7][11][11] == 0);

    // check some other points as well
    // x=10, z=10 line, from z=6 it looks like x o o o o o o o x
    CHECK(g[10][6][10] == 0);
    CHECK(g[10][7][10] == 'a');
    CHECK(g[10][8][10] == 'a');
    CHECK(g[10][9][10] == 'a');
    CHECK(g[10][10][10] == 'A');
    CHECK(g[10][11][10] == 'a');
    CHECK(g[10][12][10] == 'a');
    CHECK(g[10][13][10] == 'a');
    CHECK(g[10][14][10] == 0);

    // x=10, y=10 line, from y=6 it looks like x o o o o o o o x
    CHECK(g[10][10][6] == 0);
    CHECK(g[10][10][7] == 'a');
    CHECK(g[10][10][8] == 'a');
    CHECK(g[10][10][9] == 'a');
    CHECK(g[10][10][10] == 'A');
    CHECK(g[10][10][11] == 'a');
    CHECK(g[10][10][12] == 'a');
    CHECK(g[10][10][13] == 'a');
    CHECK(g[10][10][14] == 0);

    // some random points
    CHECK(g[9][9][9] == 'a');
    CHECK(g[8][8][8] == 0);
    CHECK(g[13][13][13] == 0);
}

TEST_CASE("volume", "[grid]") {
    Vector3 base(-10, -10, -10);
    double width = 1;
    int bins = 21;
    int radius = 1;
    Grid grid(base, width, bins, radius); 

    // cout << grid.get_volume() << endl;
    vector<Atom> a = {Atom({0, 0, 0}, 0, "C", "", 0)};
    vector<Hetatom> w = {Hetatom({2, 2, 2}, 0, "C", "", 0), Hetatom({2, 2, 3}, 0, "C", "", 0)};
    grid.add(a);
    grid.add(w);
    REQUIRE(grid.volume == 1); // atoms are added as point-particles, and only occupy one unit of space.

    grid.expand_volume();
    REQUIRE(grid.volume == 7); // the radius is 1, so expanding the volume in a sphere results in one unit of volume added along each coordinate axis

    grid.add(Atom({0, 0, -1}, 0, "C", "", 0));
    grid.expand_volume();
    REQUIRE(grid.volume == 12); // second atom is placed adjacent to the first one, so the volumes overlap. 
}

TEST_CASE("hydrate", "[grid]") {
    Vector3 base(-10, -10, -10);
    double width = 1;
    int bins = 21;
    int radius = 3;
    Grid grid(base, width, bins, radius);

    // add a single atom to the grid, and hydrate it
    setting::grid::percent_water = 0;
    vector<Atom> a = {Atom({0, 0, 0}, 0, "C", "", 0)};
    grid.add(a);
    REQUIRE(grid.get_atoms().size() == 1); // check get_protein_atoms

    vector<Hetatom> water = grid.hydrate();

    REQUIRE(water.size() == 6); // check that the expected number of water molecules was placed
    if (water.size() == 6) { // avoid crashing if the above fails
        REQUIRE(water[0].coords == Vector3{-6, 0, 0}); // (-2r, 0, 0)
        REQUIRE(water[1].coords == Vector3{6, 0, 0}); // (2r, 0, 0)
        REQUIRE(water[2].coords == Vector3{0, -6, 0}); // (0, -2r, 0)
        REQUIRE(water[3].coords == Vector3{0, 6, 0}); // (0, 2r, 0)
        REQUIRE(water[4].coords == Vector3{0, 0, -6}); // (0, 0, -2r)
        REQUIRE(water[5].coords == Vector3{0, 0, 6}); // (0, 0, 2r)
    }

    REQUIRE(grid.get_atoms().size() == 1); // check that they are indeed registered as water molecules
}

TEST_CASE("width", "[grid]") {
    Vector3 base(-10, -10, -10);
    double width = 0.1;
    int bins = 210;
    int radius = 3;
    Grid grid(base, width, bins, radius);

    vector<Atom> a = {Atom({0, 0, 0}, 0, "C", "", 0)};
    grid.add(a);
    vector<vector<vector<char>>> &g = grid.grid;

    // check that it was placed correctly
    REQUIRE(g[100][100][100] == 'A');

    // check water generation
    setting::grid::percent_water = 0;
    vector<Hetatom> water = grid.hydrate();
    REQUIRE(water.size() == 6);
    if (water.size() == 6) { // avoid crashing if the above fails
        REQUIRE(water[0].coords == Vector3{-6, 0, 0}); // (-2r, 0, 0)
        REQUIRE(water[1].coords == Vector3{6, 0, 0}); // (2r, 0, 0)
        REQUIRE(water[2].coords == Vector3{0, -6, 0}); // (0, -2r, 0)
        REQUIRE(water[3].coords == Vector3{0, 6, 0}); // (0, 2r, 0)
        REQUIRE(water[4].coords == Vector3{0, 0, -6}); // (0, 0, -2r)
        REQUIRE(water[5].coords == Vector3{0, 0, 6}); // (0, 0, 2r)
    }

    // test bounds
    grid = Grid(base, width, bins, 3);

    a = {Atom({5, 0, -7}, 0, "C", "", 1), Atom({0, -5, 0}, 0, "C", "", 2)};
    grid.add(a);
    grid.expand_volume();
    g = grid.grid;

    vector<vector<int>> box = grid.bounding_box();
    REQUIRE(box[0][0] == 100);
    REQUIRE(box[0][1] == 151);
    REQUIRE(box[1][0] == 50);
    REQUIRE(box[1][1] == 101);
    REQUIRE(box[2][0] == 30);
    REQUIRE(box[2][1] == 101);
}

TEST_CASE("add & remove", "[grid]") {
    Vector3 base(-10, -10, -10);
    double width = 1;
    int bins = 21;
    int radius = 3;
    Grid grid(base, width, bins, radius);

    // atoms
    Atom a1 = Atom({3, 0, 0}, 0, "C", "", 1);
    Atom a2 = Atom({0, 3, 0}, 0, "C", "", 2);
    Atom a3 = Atom({0, 0, 3}, 0, "C", "", 3);
    vector<Atom> a = {a1, a2, a3};

    // waters
    Hetatom w1 = Hetatom::create_new_water(Vector({0, 0, -3}));
    Hetatom w2 = Hetatom::create_new_water(Vector({0, -3, 0}));
    Hetatom w3 = Hetatom::create_new_water(Vector({-3, 0, 0}));
    vector<Hetatom> w = {w1, w2, w3};

    SECTION("add") {
        // add atoms
        grid.add(a);    
        vector<Atom> ga = grid.get_atoms();
        REQUIRE(grid.a_members.size() == 3); // check actual data
        REQUIRE(ga.size() >= 3);
        for (int i = 0; i < 3; i++) {
            REQUIRE(ga[i] == a[i]); // check equality with what we added
        }

        // add waters
        grid.add(w);
        vector<Hetatom> wa = grid.get_waters();
        REQUIRE(grid.a_members.size() == 3); // check actual data
        REQUIRE(grid.w_members.size() == 3); // check actual data
        REQUIRE(wa.size() >= 3);
        for (int i = 0; i < 3; i++) {
            REQUIRE(wa[i] == w[i]); // check equality with what we added
        }
    }

    SECTION("remove") {
        grid.add(a);
        grid.add(w);
        vector<Atom> ga = grid.get_atoms();
        vector<Hetatom> wa = grid.get_waters();

        grid.remove(a2);
        grid.remove(w3);
        grid.remove(w1);
        ga = grid.get_atoms();
        wa = grid.get_waters();

        // check sizes
        REQUIRE(ga.size() == 2);
        REQUIRE(wa.size() == 1);

        // check remaining atoms
        REQUIRE(ga[0] == a1);
        REQUIRE(ga[1] == a3);
        REQUIRE(wa[0] == w2);

        // check old centers
        vector<vector<vector<char>>> &g = grid.grid;
        vector<int> loc_a2 = grid.to_bins(a2.coords);
        vector<int> loc_w1 = grid.to_bins(w1.coords);
        vector<int> loc_w3 = grid.to_bins(w3.coords);
        REQUIRE(g[loc_a2[0]][loc_a2[1]][loc_a2[2]] == 0);
        REQUIRE(g[loc_w1[0]][loc_w1[1]][loc_w1[2]] == 0);
        REQUIRE(g[loc_w3[0]][loc_w3[1]][loc_w3[2]] == 0);
    }
}

// Test that the correct locations are found by find_free_locs. 
void test_find_free_locs(setting::grid::PlacementStrategyChoice ch) {
    Vector3 base(-10, -10, -10);
    double width = 1;
    vector<int> bins = {21, 21, 21};
    int radius = 3;
    Grid grid(base, width, bins, radius, radius, ch, setting::grid::csc);

    vector<Atom> a = {Atom({0, 0, 0}, 0, "C", "", 0)};
    grid.add(a);
    grid.expand_volume();

    vector<GridMember<Hetatom>> locs = grid.find_free_locs();
    REQUIRE(locs.size() == 6);
    // for (int i = 0; i < locs.size(); i++) {
    //     cout << format("%1%\t%2%\t%3%") % locs[i].coords.x % locs[i].coords.y % locs[i].coords.z << endl;
    // }

    // since this needs to work with different placement strategies, we have to perform a more general check on the positions
    vector<Vector3> v = {{0, 0, 6}, {0, 0, -6}, {6, 0, 0}, {-6, 0, 0}, {0, 6, 0}, {0, -6, 0}};
    if (locs.size() >= 6) { // avoid crashing if the above fails
        for (const auto& l : locs) {
            bool found = false;
            for (const auto& p : v) {
                if (l.atom.coords == p) {found = true;}
            }
            REQUIRE(found);
        }
    }
}

TEST_CASE("find_free_locs", "[grid]") {
    test_find_free_locs(setting::grid::AxesStrategy);
    test_find_free_locs(setting::grid::RadialStrategy);
}

// Test that expansion and deflation completely cancels each other. 
TEST_CASE("volume_deflation", "[grid]") {
    Vector3 base(-10, -10, -10);
    double width = 1;
    int bins = 21;
    int radius = 3;
    Grid grid(base, width, bins, radius);

    vector<Atom> a = {Atom({3, 0, 0}, 0, "C", "", 1), Atom({0, 3, 0}, 0, "C", "", 2)};
    grid.add(a);
    REQUIRE(grid.volume == 2);

    grid.expand_volume();
    grid.deflate_volume();
    vector<vector<vector<char>>> &g = grid.grid;
    REQUIRE(grid.volume == 2);

    for (int i = 0; i < bins; i++) {
        for (int j = 0; j < bins; j++) {
            for (int k = 0; k < bins; k++) {
                if (__builtin_expect(i == 10 && j == 13 && k == 10, false)) {continue;} // center of the first atom
                if (__builtin_expect(i == 13 && j == 10 && k == 10, false)) {continue;} // center of the second atom
                REQUIRE(g[i][j][k] == 0);
            }
        }
    }
}