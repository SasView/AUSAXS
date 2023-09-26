#include <catch2/catch_test_macros.hpp>

#include <hist/Histogram.h>
#include <dataset/SimpleDataset.h>

TEST_CASE("Histogram::Histogram") {
    SECTION("default") {
        hist::Histogram hist;
        CHECK(hist.size() == 0);
        CHECK(hist.span_y() == Limit{0, 0});
    }

    SECTION("Vector<double>") {
        std::vector<double> data{1, 2, 3, 4, 5};
        hist::Histogram hist(data);
        CHECK(hist.size() == 5);
        CHECK(hist.span_y() == Limit{1, 5});
    }

    SECTION("Vector<double>, Axis") {
        std::vector<double> data{1, 2, 3, 4, 5};
        Axis axis(1, 10, 5);
        hist::Histogram hist(data, axis);
        CHECK(hist.size() == 5);
        CHECK(hist.span_y() == Limit{1, 5});
    }

    SECTION("Axis") {
        Axis axis(1, 10, 10);
        hist::Histogram hist(axis);
        CHECK(hist.size() == 10);
        CHECK(hist.span_y() == Limit{0, 0});
    }
}

TEST_CASE("Histogram::shorten_axis") {
    SECTION("empty") {
        hist::Histogram hist;
        hist.shorten_axis();
        CHECK(hist.size() == 0);
        CHECK(hist.span_y() == Limit{0, 0});
    }

    SECTION("more than 10 elements") {
        std::vector<double> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0, 0, 0, 0, 0, 0, 0};
        hist::Histogram hist(data);
        hist.shorten_axis(10);
        CHECK(hist.size() == 11);
        CHECK(hist.span_y() == Limit{1, 11});
    }

    SECTION("less than 10 elements") {
        std::vector<double> data = {1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0};
        hist::Histogram hist(data);
        hist.shorten_axis(10);
        CHECK(hist.size() == 10);
        CHECK(hist.span_y() == Limit{0, 6});
    }
}

TEST_CASE("Histogram::extend_axis") {}

TEST_CASE("Histogram::resize") {
    hist::Histogram hist;
    hist.resize(10);
    CHECK(hist.size() == 10);
    CHECK(hist.get_counts().size() == 10);
}

TEST_CASE("Histogram::generate_axis") {
    std::vector<double> data{1, 2, 3, 4, 5};
    hist::Histogram hist(data);
    hist.generate_axis();
    CHECK(hist.get_axis().limits() == Limit{0, 5});
}

TEST_CASE("Histogram::set_axis") {
    std::vector<double> data{1, 2, 3, 4, 5};
    hist::Histogram hist(data);
    hist.set_axis(Axis(1, 10, 1));
    CHECK(hist.get_axis().limits() == Limit{1, 10});
}

TEST_CASE("Histogram::limits") {
    SECTION("empty") {
        hist::Histogram hist;
        CHECK(hist.span_y() == Limit{0, 0});
    }

    SECTION("non-empty") {
        std::vector<double> data{-1, 0, 1, 2, 3, 4, 5};
        hist::Histogram hist(data);
        CHECK(hist.span_y() == Limit{-1, 5});
    }
}

TEST_CASE("Histogram::limits_positive") {
    SECTION("empty") {
        hist::Histogram hist;
        CHECK(hist.span_y_positive() == Limit{0, 0});
    }

    SECTION("non-empty") {
        std::vector<double> data{-1, 0, 1, 2, 3, 4, 5};
        hist::Histogram hist(data);
        CHECK(hist.span_y_positive() == Limit{0, 5});
    }
}

TEST_CASE("Histogram::size") {
    SECTION("empty") {
        hist::Histogram hist;
        CHECK(hist.size() == 0);
    }

    SECTION("non-empty") {
        std::vector<double> data{-1, 0, 1, 2, 3, 4, 5};
        hist::Histogram hist(data);
        CHECK(hist.size() == data.size());
    }
}

TEST_CASE("Histogram::as_dataset") {
    SECTION("empty") {
        hist::Histogram hist;
        auto dataset = hist.as_dataset();
        CHECK(dataset.size() == 0);
    }

    SECTION("non-empty") {
        std::vector<double> data{0, 1, 2, 3, 4, 5, 6};
        hist::Histogram hist(data);
        auto dataset = hist.as_dataset();
        CHECK(dataset.size() == data.size());
        CHECK(dataset.x() == data);
        CHECK(dataset.y() == hist.get_counts());
    }
}

TEST_CASE("Histogram::operator+=") {
    std::vector<double> data1{1, 2, 3, 4, 5};
    std::vector<double> data2{1, 2, 3, 4, 5};
    hist::Histogram hist1(data1);
    hist::Histogram hist2(data2);
    hist1 += hist2;
    CHECK(hist1.size() == 5);
    CHECK(hist1.span_y() == Limit{2, 10});
    CHECK(hist1.get_counts() == std::vector<double>{2, 4, 6, 8, 10});
}

TEST_CASE("Histogram::operator-=") {
    std::vector<double> data1{1, 2, 3, 4, 5};
    std::vector<double> data2{1, 2, 3, 4, 5};
    hist::Histogram hist1(data1);
    hist::Histogram hist2(data2);
    hist1 -= hist2;
    CHECK(hist1.size() == 5);
    CHECK(hist1.span_y() == Limit{0, 0});
    CHECK(hist1.get_counts() == std::vector<double>{0, 0, 0, 0, 0});
}

TEST_CASE("Histogram::operator*=") {
    std::vector<double> data1{1, 2, 3, 4, 5};
    hist::Histogram hist1(data1);
    hist1 *= 2;
    CHECK(hist1.size() == 5);
    CHECK(hist1.span_y() == Limit{2, 10});
    CHECK(hist1.get_counts() == std::vector<double>{2, 4, 6, 8, 10});
}

TEST_CASE("Histogram::operator[]") {
    std::vector<double> data{1, 2, 3, 4, 5};
    hist::Histogram hist(data);
    CHECK(hist[0] == 1);
    CHECK(hist[1] == 2);
    CHECK(hist[2] == 3);
    CHECK(hist[3] == 4);
    CHECK(hist[4] == 5);
}

TEST_CASE("Histogram::operator==") {
    std::vector<double> data1{1, 2, 3, 4, 5};
    std::vector<double> data2{1, 2, 3, 4, 5};
    hist::Histogram hist1(data1);
    hist::Histogram hist2(data2);
    CHECK(hist1 == hist2);

    std::vector<double> data3{1, 2, 3, 4, 6};
    hist::Histogram hist3(data3);
    CHECK(hist2 != hist3);
}