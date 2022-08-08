#include <hydrate/JanPlacement.h>
#include <hydrate/Grid.h>

using std::vector;

vector<grid::GridMember<Hetatom>> grid::JanPlacement::place() const {
    // dereference the values we'll need for better performance
    vector<vector<vector<char>>>& gref = grid->grid;
    auto bins = grid->get_bins();

    // place a water molecule (note: not added to the grid before the end of this method)
    vector<Hetatom> placed_water(grid->a_members.size());
    size_t index = 0;
    auto add_loc = [&] (const Vector3<int>& v) {
        Hetatom a = Hetatom::create_new_water(grid->to_xyz(v));
        if (__builtin_expect(placed_water.size() <= index, false)) {
            placed_water.resize(2*index);
        }
        placed_water[index++] = a;
    };

    // loop over the location of all member atoms
    int r_eff = grid->ra + grid->rh;
    auto[min, max] = grid->bounding_box_index();
    for (int i = min.x(); i < max.x(); i++) {
        for (int j = min.y(); j < max.y(); j++) {
            for (int k = min.z(); k < max.z(); k++) {
                if (gref[i][j][k] == 0) {continue;}

                // we define a small box of size [i-rh, i+rh][j-rh, j+rh][z-rh, z+rh]
                int im = std::max(i-r_eff, 0), ip = std::min(i+r_eff, (int) bins.x()-1); // xminus and xplus
                int jm = std::max(j-r_eff, 0), jp = std::min(j+r_eff, (int) bins.y()-1); // yminus and yplus
                int km = std::max(k-r_eff, 0), kp = std::min(k+r_eff, (int) bins.z()-1); // zminus and zplus

                // check collisions for x ± r_eff                
                if (gref[im][j][k] == 0) {add_loc(Vector3<int>(im, j, k));}
                if (gref[ip][j][k] == 0) {add_loc(Vector3<int>(ip, j, k));}

                // check collisions for y ± r_eff
                if (gref[i][jp][k] == 0) {add_loc(Vector3<int>(i, jp, k));}
                if (gref[i][jm][k] == 0) {add_loc(Vector3<int>(i, jm, k));}

                // check collisions for z ± r_eff
                if (gref[i][j][km] == 0) {add_loc(Vector3<int>(i, j, km));}
                if (gref[i][j][kp] == 0) {add_loc(Vector3<int>(i, j, kp));}
            }
        }
    }

    // finally we can add the atoms to the grid
    placed_water.resize(index);
    vector<grid::GridMember<Hetatom>> v = grid->add(placed_water);
    grid->expand_volume();

    return v;
}