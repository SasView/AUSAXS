#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <form_factor/FormFactor.h>
#include <form_factor/FormFactorTable.h>
#include <constants/Constants.h>

#include <iostream>

using namespace form_factor;

// Check that we have the correct conversion of the s-values. The form factors are not supposed to change a lot over the span of our q-values.
TEST_CASE("FormFactor::evaluate") {
    for (unsigned int ff = 0; ff < get_count_without_excluded_volume(); ++ff) {
        const FormFactor& ff_obj = storage::atomic::get_form_factor(static_cast<form_factor_t>(ff));
        CHECK_THAT(ff_obj.evaluate(0.0), Catch::Matchers::WithinAbs(1, 1e-6));
        if (ff_obj.evaluate(0.5) < 0.95) {
            std::cout << "Warning: Form factor " << ff << " has a value of " << ff_obj.evaluate(0.5) << " at q = 0.5" << std::endl;
            FAIL();
        }
        SUCCEED();
    }
}

// Check that the form factors are normalized. 
TEST_CASE("FormFactor::normalized") {
    for (unsigned int ff = 0; ff < get_count_without_excluded_volume(); ++ff) {
        const FormFactor& ff_obj = storage::atomic::get_form_factor(static_cast<form_factor_t>(ff));
        CHECK_THAT(ff_obj.evaluate(0), Catch::Matchers::WithinAbs(1, 1e-6));
    }
}

// Compare our five-Gaussian form factors with the more typical four-Gaussian form factors.
// These are all taken from the table at https://lampx.tugraz.at/~hadley/ss1/crystaldiffraction/atomicformfactors/formfactors.php (International Tables for Crystallography)
using constants::form_factor::s_to_q;
const auto& q_vals = constants::axes::q_vals;
TEST_CASE("FormFactor::compare_with_four_gaussians") {
    SECTION("oxygen") {
        std::array<double, 5> a =        {3.0485,  2.2868, 1.5463, 0.867,   0};
        std::array<double, 5> b = s_to_q({13.2771, 5.7011, 0.3239, 32.9089, 0});
        double c = 0.2508;

        FormFactor ff(a, b, c);
        const FormFactor& O = storage::atomic::get_form_factor(form_factor_t::O);
        for (const double& q : q_vals) {
            CHECK_THAT(ff.evaluate(q), Catch::Matchers::WithinAbs(O.evaluate(q), 1e-3));
        }
    }

    SECTION("nitrogen") {
        std::array<double, 5> a =        {12.2126, 3.1322, 2.0125,  1.1663, 0};
        std::array<double, 5> b = s_to_q({0.0057,  9.8933, 28.9975, 0.5826, 0});
        double c = -11.529;

        FormFactor ff(a, b, c);
        const FormFactor& N = storage::atomic::get_form_factor(form_factor_t::N);
        for (const double& q : q_vals) {
            CHECK_THAT(ff.evaluate(q), Catch::Matchers::WithinAbs(N.evaluate(q), 1e-3));
        }
    }

    SECTION("carbon") {
        std::array<double, 5> a =        {2.31,    1.02,    1.5886, 0.865,   0};
        std::array<double, 5> b = s_to_q({20.8439, 10.2075, 0.5687, 51.6512, 0});
        double c = 0.2156;

        FormFactor ff(a, b, c);
        const FormFactor& C = storage::atomic::get_form_factor(form_factor_t::C);
        for (const double& q : q_vals) {
            CHECK_THAT(ff.evaluate(q), Catch::Matchers::WithinAbs(C.evaluate(q), 1e-3));
        }
    }

    SECTION("argon") {
        std::array<double, 5> a =        {7.4845, 6.7723,  0.6539,  1.6442,  0};
        std::array<double, 5> b = s_to_q({0.9072, 14.8407, 43.8983, 33.3929, 0});
        double c = 1.4445;

        FormFactor ff(a, b, c);
        const FormFactor& other = storage::atomic::get_form_factor(form_factor_t::OTHER);
        for (const double& q : q_vals) {
            CHECK_THAT(ff.evaluate(q), Catch::Matchers::WithinAbs(other.evaluate(q), 1e-3));
        }
    }

    SECTION("sulphur") {
        std::array<double, 5> a =        {6.9053, 5.2034,  1.4379, 1.5863, 0};
        std::array<double, 5> b = s_to_q({1.4679, 22.2151, 0.2536, 56.172, 0});
        double c = 0.8669;

        FormFactor ff(a, b, c);
        const FormFactor& S = storage::atomic::get_form_factor(form_factor_t::S);
        for (const double& q : q_vals) {
            CHECK_THAT(ff.evaluate(q), Catch::Matchers::WithinAbs(S.evaluate(q), 1e-3));
        }
    }
}