#include <catch2/catch_test_macros.hpp>

#include <dataset/detail/DATConstructor.h>
#include <dataset/detail/XVGConstructor.h>
#include <dataset/Dataset.h>
#include <math/Vector.h>
#include <settings/GeneralSettings.h>

#include <fstream>

TEST_CASE("DATConstructor::construct") {
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
            auto data = detail::DATConstructor().construct(test_file, 0);
            REQUIRE(data->M == 3);
            CHECK(data->col(0) == Vector<double>({0.1, 0.2, 0.3}));
            CHECK(data->col(1) == Vector<double>({1, 2, 3}));
            CHECK(data->col(2) == Vector<double>({10, 20, 30}));

            // M = 1
            auto data1 = detail::DATConstructor().construct(test_file, 1);
            REQUIRE(data1->M == 1);
            CHECK(data1->col(0) == Vector<double>({0.1, 0.2, 0.3}));

            // M = 2
            auto data2 = detail::DATConstructor().construct(test_file, 2);
            REQUIRE(data2->M == 2);
            CHECK(data2->col(0) == Vector<double>({0.1, 0.2, 0.3}));
            CHECK(data2->col(1) == Vector<double>({1, 2, 3}));

            // M = 3
            auto data3 = detail::DATConstructor().construct(test_file, 3);
            REQUIRE(data3->M == 3);
            CHECK(data3->col(0) == Vector<double>({0.1, 0.2, 0.3}));
            CHECK(data3->col(1) == Vector<double>({1, 2, 3}));
            CHECK(data3->col(2) == Vector<double>({10, 20, 30}));
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
            auto data = detail::DATConstructor().construct(test_file, 0);
            REQUIRE(data->M == 4);
            CHECK(data->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));
            CHECK(data->col(1) == Vector<double>({1,   2,   3,   4}));
            CHECK(data->col(2) == Vector<double>({10,  20,  30,  40}));
            CHECK(data->col(3) == Vector<double>({100, 200, 300, 400}));

            // M = 1
            auto data1 = detail::DATConstructor().construct(test_file, 1);
            REQUIRE(data1->M == 1);
            CHECK(data1->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));

            // M = 2
            auto data2 = detail::DATConstructor().construct(test_file, 2);
            REQUIRE(data2->M == 2);
            CHECK(data2->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));
            CHECK(data2->col(1) == Vector<double>({1,   2,   3,   4}));

            // M = 3
            auto data3 = detail::DATConstructor().construct(test_file, 3);
            REQUIRE(data3->M == 3);
            CHECK(data3->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));
            CHECK(data3->col(1) == Vector<double>({1,   2,   3,   4}));
            CHECK(data3->col(2) == Vector<double>({10,  20,  30,  40}));

            // M = 4
            auto data4 = detail::DATConstructor().construct(test_file, 4);
            REQUIRE(data4->M == 4);
            CHECK(data4->col(0) == Vector<double>({0.1, 0.2, 0.3, 0.4}));
            CHECK(data4->col(1) == Vector<double>({1,   2,   3,   4}));
            CHECK(data4->col(2) == Vector<double>({10,  20,  30,  40}));
            CHECK(data4->col(3) == Vector<double>({100, 200, 300, 400}));
        }
    }
}

TEST_CASE("XVGConstructor::construct") {
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
            auto data = detail::XVGConstructor().construct(test_file, 0);
            REQUIRE(data->M == 3);
            CHECK(data->col(0) == Vector<double>({0.01, 0.02, 0.03}));
            CHECK(data->col(1) == Vector<double>({1,    2,    3}));
            CHECK(data->col(2) == Vector<double>({10,   20,   30}));

            // M = 1
            auto data1 = detail::XVGConstructor().construct(test_file, 1);
            REQUIRE(data1->M == 1);
            CHECK(data1->col(0) == Vector<double>({0.01, 0.02, 0.03}));

            // M = 2
            auto data2 = detail::XVGConstructor().construct(test_file, 2);
            REQUIRE(data2->M == 2);
            CHECK(data2->col(0) == Vector<double>({0.01, 0.02, 0.03}));
            CHECK(data2->col(1) == Vector<double>({1,    2,    3}));

            // M = 3
            auto data3 = detail::XVGConstructor().construct(test_file, 3);
            REQUIRE(data3->M == 3);
            CHECK(data3->col(0) == Vector<double>({0.01, 0.02, 0.03}));
            CHECK(data3->col(1) == Vector<double>({1,    2,    3}));
            CHECK(data3->col(2) == Vector<double>({10,   20,   30}));
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
            auto data = detail::XVGConstructor().construct(test_file, 0);
            REQUIRE(data->M == 4);
            CHECK(data->col(0) == Vector<double>({0.01, 0.02, 0.03, 0.04}));
            CHECK(data->col(1) == Vector<double>({1,    2,    3,    4}));
            CHECK(data->col(2) == Vector<double>({10,   20,   30,   40}));
            CHECK(data->col(3) == Vector<double>({100,  200,  300,  400}));

            // M = 1
            auto data1 = detail::XVGConstructor().construct(test_file, 1);
            REQUIRE(data1->M == 1);
            CHECK(data->col(0) == Vector<double>({0.01, 0.02, 0.03, 0.04}));

            // M = 2
            auto data2 = detail::XVGConstructor().construct(test_file, 2);
            REQUIRE(data2->M == 2);
            CHECK(data2->col(0) == Vector<double>({0.01, 0.02, 0.03, 0.04}));
            CHECK(data2->col(1) == Vector<double>({1,    2,    3,    4}));

            // M = 3
            auto data3 = detail::XVGConstructor().construct(test_file, 3);
            REQUIRE(data3->M == 3);
            CHECK(data3->col(0) == Vector<double>({0.01, 0.02, 0.03, 0.04}));
            CHECK(data3->col(1) == Vector<double>({1,    2,    3,    4}));
            CHECK(data3->col(2) == Vector<double>({10,   20,   30,   40}));

            // M = 4
            auto data4 = detail::XVGConstructor().construct(test_file, 4);
            REQUIRE(data4->M == 4);
            CHECK(data4->col(0) == Vector<double>({0.01, 0.02, 0.03, 0.04}));
            CHECK(data4->col(1) == Vector<double>({1,    2,    3,    4}));
            CHECK(data4->col(2) == Vector<double>({10,   20,   30,   40}));
            CHECK(data4->col(3) == Vector<double>({100,  200,  300,  400}));
        }
    }
}