#include <data/symmetry/detail/SymmetryHelpers.h>
#include <data/Molecule.h>
#include <data/Body.h>

using namespace ausaxs;
using namespace ausaxs::symmetry::detail;
using namespace ausaxs::hist::detail;

#define DEBUG_MODE false
#if DEBUG_MODE
    #include <iostream>
    #include <iomanip>
#endif
std::vector<CompactCoordinateSymmetries> ausaxs::symmetry::detail::generate_transformed_data(const data::Molecule& protein) {
    std::vector<CompactCoordinateSymmetries> res(protein.size_body());
    for (int i_body1 = 0; i_body1 < static_cast<int>(protein.size_body()); ++i_body1) {
        res[i_body1] = generate_transformed_data(protein.get_body(i_body1));
    }
    return res;
}

CompactCoordinateSymmetries ausaxs::symmetry::detail::generate_transformed_data(const data::Body& body) {
    CompactCoordinates data_a(body.get_atoms());
    CompactCoordinates data_w;
    if (body.size_water() != 0) {
        data_w = CompactCoordinates(body.get_waters());
    } 

    std::vector<std::vector<CompactCoordinates>> atomic(1+body.size_symmetry());
    std::vector<std::vector<CompactCoordinates>> water(1+body.size_symmetry());

    #if DEBUG_MODE
        for (int i = 0; i < static_cast<int>(data_a.get_data().size()); ++i) {
            std::cout << "before: " << data_a.get_data()[i].value.pos << std::endl;
        }
    #endif

    // loop over its symmetries
    for (int i_sym_1 = 0; i_sym_1 < static_cast<int>(body.size_symmetry()); ++i_sym_1) {
        const auto& symmetry = body.symmetry().get(i_sym_1);

        std::vector<CompactCoordinates> sym_atomic(symmetry.repeat, data_a);
        std::vector<CompactCoordinates> sym_water(symmetry.repeat, data_w);

        // for every symmetry, loop over how many times it should be repeated
        // it is then repeatedly applied to the same data
        for (int i_repeat = 0; i_repeat < symmetry.repeat; ++i_repeat) {
            auto t = symmetry.get_transform<double>(i_repeat+1);
            std::transform(
                sym_atomic[i_repeat].get_data().begin(), 
                sym_atomic[i_repeat].get_data().end(), 
                sym_atomic[i_repeat].get_data().begin(), 
                [t] (const CompactCoordinatesData& v) -> CompactCoordinatesData {return {t(v.value.pos), v.value.w}; }
            );
            std::transform(
                sym_water[i_repeat].get_data().begin(), 
                sym_water[i_repeat].get_data().end(), 
                sym_water[i_repeat].get_data().begin(), 
                [t] (const CompactCoordinatesData& v) -> CompactCoordinatesData {return {t(v.value.pos), v.value.w}; }
            );

            #if DEBUG_MODE
                for (int i = 0; i < static_cast<int>(sym_atomic[i_repeat].get_data().size()); ++i) {
                    std::cout << "after: " << sym_atomic[i_repeat].get_data()[i].value.pos << std::endl;
                }
            #endif
        }
        atomic[1+i_sym_1] = std::move(sym_atomic);
        water [1+i_sym_1] = std::move(sym_water);
    }
    atomic[0]    = {std::move(data_a)};
    water[0]     = {std::move(data_w)};
    return {std::move(atomic), std::move(water)};
}

std::vector<std::vector<hist::detail::CompactCoordinates>> ausaxs::symmetry::detail::generate_transformed_waters(const data::Body& body) {
    if (body.size_water() == 0) {
        return {{CompactCoordinates{}}};
    } 
    CompactCoordinates data_w = CompactCoordinates(body.get_waters());
    std::vector<std::vector<CompactCoordinates>> water(1+body.size_symmetry());

    // loop over its symmetries
    for (int i_sym_1 = 0; i_sym_1 < static_cast<int>(body.size_symmetry()); ++i_sym_1) {
        const auto& symmetry = body.symmetry().get(i_sym_1);
        std::vector<CompactCoordinates> sym_water(symmetry.repeat, data_w);

        // for every symmetry, loop over how many times it should be repeated
        // it is then repeatedly applied to the same data
        for (int i_repeat = 0; i_repeat < symmetry.repeat; ++i_repeat) {
            auto t = symmetry.get_transform<double>(i_repeat+1);
            std::transform(
                sym_water[i_repeat].get_data().begin(), 
                sym_water[i_repeat].get_data().end(), 
                sym_water[i_repeat].get_data().begin(), 
                [t] (const CompactCoordinatesData& v) -> CompactCoordinatesData {return {t(v.value.pos), v.value.w}; }
            );
       }
        water[1+i_sym_1] = std::move(sym_water);
    }
    water[0] = {std::move(data_w)};
    return water;
}