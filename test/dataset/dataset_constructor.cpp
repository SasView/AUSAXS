#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <dataset/detail/DATReader.h>
#include <dataset/detail/XVGReader.h>
#include <dataset/Dataset.h>
#include <math/Vector.h>
#include <settings/GeneralSettings.h>

#include <fstream>

TEST_CASE("DATReader::construct") {
    settings::general::verbose = false;
    std::string test_file = "temp/dataset/dat_test.dat";

    SECTION("simple contents") {
        std::string test_file_contents = 
            "x y z\n"
            "0.1 1 10\n"
            "0.2 2 20\n"
            "0.3 3 30\n";

        std::ofstream file(test_file);
        file << test_file_contents;
        file.close();

        // default M
        auto data = detail::DATReader().construct(test_file, 0);
        REQUIRE(data->M == 3);
        CHECK(data->col(0) == Vector<double>({0.1, 0.2, 0.3}));
        CHECK(data->col(1) == Vector<double>({1, 2, 3}));
        CHECK(data->col(2) == Vector<double>({10, 20, 30}));

        // M = 1
        auto data1 = detail::DATReader().construct(test_file, 1);
        REQUIRE(data1->M == 1);
        CHECK(data1->col(0) == Vector<double>({0.1, 0.2, 0.3}));

        // M = 2
        auto data2 = detail::DATReader().construct(test_file, 2);
        REQUIRE(data2->M == 2);
        CHECK(data2->col(0) == Vector<double>({0.1, 0.2, 0.3}));
        CHECK(data2->col(1) == Vector<double>({1, 2, 3}));

        // M = 3
        auto data3 = detail::DATReader().construct(test_file, 3);
        REQUIRE(data3->M == 3);
        CHECK(data3->col(0) == Vector<double>({0.1, 0.2, 0.3}));
        CHECK(data3->col(1) == Vector<double>({1, 2, 3}));
        CHECK(data3->col(2) == Vector<double>({10, 20, 30}));
    }

    SECTION("weird contents") {
        std::string test_file_contents = 
            "x y z\n"
            "0.1 1 10 100\n"
            "0.11 1.1 11\n"
            "0.12 1.2\n"
            "skip me\n"
            "0.2 2 20 200\n"
            "0.3 3 30 300\n"
            "0.4 4 40 400\n";

        {
            std::ofstream file(test_file);
            file << test_file_contents;
            file.close();

            // default M
            auto data = detail::DATReader().construct(test_file, 0);
            REQUIRE(data->M == 4);
            CHECK(data->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));
            CHECK(data->col(1) == Vector<double>({1,   2,   3,   4}));
            CHECK(data->col(2) == Vector<double>({10,  20,  30,  40}));
            CHECK(data->col(3) == Vector<double>({100, 200, 300, 400}));

            // M = 1
            auto data1 = detail::DATReader().construct(test_file, 1);
            REQUIRE(data1->M == 1);
            CHECK(data1->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));

            // M = 2
            auto data2 = detail::DATReader().construct(test_file, 2);
            REQUIRE(data2->M == 2);
            CHECK(data2->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));
            CHECK(data2->col(1) == Vector<double>({1,   2,   3,   4}));

            // M = 3
            auto data3 = detail::DATReader().construct(test_file, 3);
            REQUIRE(data3->M == 3);
            CHECK(data3->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));
            CHECK(data3->col(1) == Vector<double>({1,   2,   3,   4}));
            CHECK(data3->col(2) == Vector<double>({10,  20,  30,  40}));

            // M = 4
            auto data4 = detail::DATReader().construct(test_file, 4);
            REQUIRE(data4->M == 4);
            CHECK(data4->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));
            CHECK(data4->col(1) == Vector<double>({1,   2,   3,   4}));
            CHECK(data4->col(2) == Vector<double>({10,  20,  30,  40}));
            CHECK(data4->col(3) == Vector<double>({100, 200, 300, 400}));
        }
    }
}

auto vec_approx = [](const auto& v1, const auto& v2) {
    REQUIRE(v1.size() == v2.size());
    for (unsigned int i = 0; i < v1.size(); i++) {
        CHECK_THAT(v1[i], Catch::Matchers::WithinAbs(v2[i], 1e-6));
    }
};

TEST_CASE("XVGReader::construct") {
    settings::general::verbose = false;
    std::string test_file = "temp/dataset/dat_test.dat";

    SECTION("simple contents") {
        std::string test_file_contents = 
            "x y z\n"
            "0.1 1 10\n"
            "0.2 2 20\n"
            "0.3 3 30\n";

        {
            std::ofstream file(test_file);
            file << test_file_contents;
            file.close();

            // default M
            auto data = detail::XVGReader().construct(test_file, 0);
            REQUIRE(data->M == 3);
            vec_approx(data->col(0), std::vector<double>({0.01, 0.02, 0.03}));
            vec_approx(data->col(1), std::vector<double>({1, 2, 3}));
            vec_approx(data->col(2), std::vector<double>({10, 20, 30}));

            // M = 1
            auto data1 = detail::XVGReader().construct(test_file, 1);
            REQUIRE(data1->M == 1);
            vec_approx(data1->col(0), std::vector<double>({0.01, 0.02, 0.03}));

            // M = 2
            auto data2 = detail::XVGReader().construct(test_file, 2);
            REQUIRE(data2->M == 2);
            vec_approx(data2->col(0), std::vector<double>({0.01, 0.02, 0.03}));
            vec_approx(data2->col(1), std::vector<double>({1,    2,    3}));

            // M = 3
            auto data3 = detail::XVGReader().construct(test_file, 3);
            REQUIRE(data3->M == 3);
            vec_approx(data3->col(0), std::vector<double>({0.01, 0.02, 0.03}));
            vec_approx(data3->col(1), std::vector<double>({1,    2,    3}));
            vec_approx(data3->col(2), std::vector<double>({10,   20,   30}));
        }
    }

    SECTION("weird contents") {
        std::string test_file_contents = 
            "x y z\n"
            "0.1 1 10 100\n"
            "0.11 1.1 11\n"
            "0.12 1.2\n"
            "skip me\n"
            "0.2 2 20 200\n"
            "0.3 3 30 300\n"
            "0.4 4 40 400\n";

        {
            std::ofstream file(test_file);
            file << test_file_contents;
            file.close();

            // default M
            auto data = detail::XVGReader().construct(test_file, 0);
            REQUIRE(data->M == 4);
            vec_approx(data->col(0), std::vector<double>({0.01, 0.02, 0.03, 0.04}));
            vec_approx(data->col(1), std::vector<double>({1,    2,    3,    4}));
            vec_approx(data->col(2), std::vector<double>({10,   20,   30,   40}));
            vec_approx(data->col(3), std::vector<double>({100,  200,  300,  400}));

            // M = 1
            auto data1 = detail::XVGReader().construct(test_file, 1);
            REQUIRE(data1->M == 1);
            vec_approx(data1->col(0), std::vector<double>({0.01, 0.02, 0.03, 0.04}));

            // M = 2
            auto data2 = detail::XVGReader().construct(test_file, 2);
            REQUIRE(data2->M == 2);
            vec_approx(data2->col(0), std::vector<double>({0.01, 0.02, 0.03, 0.04}));
            vec_approx(data2->col(1), std::vector<double>({1,    2,    3,    4}));

            // M = 3
            auto data3 = detail::XVGReader().construct(test_file, 3);
            REQUIRE(data3->M == 3);
            vec_approx(data3->col(0), std::vector<double>({0.01, 0.02, 0.03, 0.04}));
            vec_approx(data3->col(1), std::vector<double>({1,    2,    3,    4}));
            vec_approx(data3->col(2), std::vector<double>({10,   20,   30,   40}));

            // M = 4
            auto data4 = detail::XVGReader().construct(test_file, 4);
            REQUIRE(data4->M == 4);
            vec_approx(data4->col(0), std::vector<double>({0.01, 0.02, 0.03, 0.04}));
            vec_approx(data4->col(1), std::vector<double>({1,    2,    3,    4}));
            vec_approx(data4->col(2), std::vector<double>({10,   20,   30,   40}));
            vec_approx(data4->col(3), std::vector<double>({100,  200,  300,  400}));
        }
    }
}