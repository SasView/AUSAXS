// includes
#include <vector>
#include <string>
#include <iostream>
#include <random>

#include "math/Matrix.h"
#include "math/Vector.h"
#include "math/Vector3.h"
#include "math/Cramer2DSolver.cpp"
#include "math/GivensSolver.cpp"
#include "math/CubicSpline.h"
#include "math/LUPDecomposition.h"
#include "math/QRDecomposition.h"
#include "Tools.h"
#include "catch2/catch.hpp"

#include <TCanvas.h>
#include <TGraph.h>

using std::cout, std::endl;

Vector GenRandVector(int m) {
    Vector v(m);
    for (int i = 0; i < m; i++)
        v[i] = rand() % 100;
    return v;
}

Matrix GenRandMatrix(int n, int m) {
    Matrix M(n, m);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            M[i][j] = rand() % 100;
    return M;
}

TEST_CASE("Vector3", "[math]") {
    Vector3 x = {1, 2, 3};
    Vector3 y = {4, 5, 6};
    Vector3 z = {7, 8, 9};

    SECTION("basic operations") {
        // access
        REQUIRE(x.x == 1);
        REQUIRE(x.y == 2);
        REQUIRE(x.z == 3);

        REQUIRE(y.x == 4);
        REQUIRE(y.y == 5);
        REQUIRE(y.z == 6);

        // addition
        REQUIRE(x+y == Vector3{5, 7, 9});
        REQUIRE(x+z == Vector3{8, 10, 12});
        REQUIRE(y+z == Vector3{11, 13, 15});

        // subtraction
        REQUIRE(x-y == Vector3{-3, -3, -3});
        REQUIRE(x-z == Vector3{-6, -6, -6});
        REQUIRE(y-z == Vector3{-3, -3, -3});

        REQUIRE(y-x == Vector3{3, 3, 3});
        REQUIRE(z-x == Vector3{6, 6, 6});
        REQUIRE(z-y == Vector3{3, 3, 3});

        // negation
        REQUIRE(-x == Vector3{-1, -2, -3});
        REQUIRE(-y == Vector3{-4, -5, -6});
        REQUIRE(-z == Vector3{-7, -8, -9});

        // dot product
        REQUIRE(x.dot(y) == 4+10+18);
        REQUIRE(x.dot(z) == 7+16+27);
        REQUIRE(y.dot(z) == 28+40+54);

        // norm
        REQUIRE(x.norm() == sqrt(1+4+9));
        REQUIRE(y.norm() == sqrt(16+25+36));
        REQUIRE(z.norm() == sqrt(49+64+81));
    }

    SECTION("assignment operators") {
        x += z;
        y += z;
        REQUIRE(x == Vector3{8, 10, 12});
        REQUIRE(y == Vector3{11, 13, 15});

        x -= z;
        y -= z;
        REQUIRE(x == Vector3{1, 2, 3});
        REQUIRE(y == Vector3{4, 5, 6});

        x = z;
        y = z;
        z = {0, 0, 0};
        REQUIRE(x == Vector3{7, 8, 9});
        REQUIRE(y == Vector3{7, 8, 9});
    }

    SECTION("cross product") {
        Vector3 v = x.cross(y);
        Vector3 w = x.cross(z);
        REQUIRE(v == Vector3{-3, 6, -3});
        REQUIRE(w == Vector3{-6, 12, -6});
    }

    SECTION("distance") {
        REQUIRE(x.distance2(y) == 27);
        REQUIRE(x.distance2(z) == 108);
        REQUIRE(y.distance2(z) == 27);

        REQUIRE(x.distance(y) == sqrt(27));
        REQUIRE(x.distance(z) == sqrt(108));
        REQUIRE(y.distance(z) == sqrt(27));
    }
}

TEST_CASE("Vector", "[math]") {
    Vector x = {1, 2, 3, 4};
    Vector y = {2, 3, 4, 5};
    Vector z = {4, 3, 2, 1};

    SECTION("basic operations") {
        // addition
        REQUIRE(x+y == Vector{3, 5, 7, 9});
        REQUIRE(x+z == Vector{5, 5, 5, 5});
        REQUIRE(y+z == Vector{6, 6, 6, 6});

        // subtraction
        REQUIRE(x-y == Vector{-1, -1, -1, -1});
        REQUIRE(x-z == Vector{-3, -1, 1, 3});
        REQUIRE(y-z == Vector{-2, 0, 2, 4});

        REQUIRE(y-x == Vector{1, 1, 1, 1});
        REQUIRE(z-x == Vector{3, 1, -1, -3});
        REQUIRE(z-y == Vector{2, 0, -2, -4});

        // negation
        REQUIRE(-x == Vector{-1, -2, -3, -4});
        REQUIRE(-y == Vector{-2, -3, -4, -5});
        REQUIRE(-z == Vector{-4, -3, -2, -1});

        // dot product
        REQUIRE(x.dot(y) == 2+6+12+20);
        REQUIRE(x.dot(z) == 4+6+6+4);
        REQUIRE(y.dot(z) == 8+9+8+5);

        // norm
        REQUIRE(x.norm() == sqrt(1+4+9+16));
        REQUIRE(y.norm() == sqrt(4+9+16+25));
        REQUIRE(z.norm() == sqrt(16+9+4+1));

        // vector multiplication
        REQUIRE(x*y == Vector{2, 6, 12, 20});
        REQUIRE(x*z == Vector{4, 6, 6, 4});
        REQUIRE(y*z == Vector{8, 9, 8, 5});

        // scalar multiplication
        REQUIRE(x*2 == Vector{2, 4, 6, 8});
        REQUIRE(y*3 == Vector{6, 9, 12, 15});
        REQUIRE(z*5 == Vector{20, 15, 10, 5});

        REQUIRE(2*x == Vector{2, 4, 6, 8});
        REQUIRE(3*y == Vector{6, 9, 12, 15});
        REQUIRE(5*z == Vector{20, 15, 10, 5});

        // scalar division
        REQUIRE(x/2 == Vector{1./2, 2./2, 3./2, 4./2});
        REQUIRE(y/4 == Vector{2./4, 3./4, 4./4, 5./4});
        REQUIRE(z/8 == Vector{4./8, 3./8, 2./8, 1./8});
    }

    SECTION("assignment operators") {
        x += z;
        y += z;
        REQUIRE(x == Vector{5, 5, 5, 5});
        REQUIRE(y == Vector{6, 6, 6, 6});

        x -= z;
        y -= z;
        REQUIRE(x == Vector3{1, 2, 3, 4});
        REQUIRE(y == Vector3{2, 3, 4, 5});

        x = z;
        y = z;
        z = {0, 0, 0};
        REQUIRE(x == Vector3{4, 3, 2, 1});
        REQUIRE(y == Vector3{4, 3, 2, 1});

        x = {9, 8, 7, 6, 5};
        y = {1, 2, 3, 4, 5};
        REQUIRE(x == Vector{9, 8, 7, 6, 5});
        REQUIRE(y == Vector{1, 2, 3, 4, 5});
    }

    SECTION("distance") {
        REQUIRE(x.distance2(y) == 1+1+1+1);
        REQUIRE(x.distance2(z) == 9+1+1+9);
        REQUIRE(y.distance2(z) == 4+0+4+16);

        REQUIRE(x.distance(y) == sqrt(1+1+1+1));
        REQUIRE(x.distance(z) == sqrt(9+1+1+9));
        REQUIRE(y.distance(z) == sqrt(4+0+4+16));
    }

    SECTION("iterator") {
        for (const auto& e : x) {
            REQUIRE((e == 1 || e == 2 || e == 3 || e == 4));
        }
        for (const auto& e : y) {
            REQUIRE((e == 2 || e == 3 || e == 4 || e == 5));
        }
    }
}

TEST_CASE("Matrix", "[math]") {
    Matrix A({{1, 2}, {3, 4}});
    Matrix B({{5, 6}, {7, 8}});
    Matrix C({{2, 3}, {3, 4}});

    SECTION("basic operations") {
        // addition
        REQUIRE(A+B == Matrix{{6, 8}, {10, 12}});
        REQUIRE(A+C == Matrix{{3, 5}, {6, 8}});
        REQUIRE(B+C == Matrix{{7, 9}, {10, 12}});

        // subtraction
        REQUIRE(A-B == Matrix{{-4, -4}, {-4, -4}});
        REQUIRE(A-C == Matrix{{-1, -1}, {0, 0}});
        REQUIRE(B-C == Matrix{{3, 3}, {4, 4}});

        REQUIRE(B-A == Matrix{{4, 4}, {4, 4}});
        REQUIRE(C-A == Matrix{{1, 1}, {0, 0}});
        REQUIRE(C-B == Matrix{{-3, -3}, {-4, -4}});

        // negation
        REQUIRE(-A == Matrix{{-1, -2}, {-3, -4}});
        REQUIRE(-B == Matrix{{-5, -6}, {-7, -8}});
        REQUIRE(-C == Matrix{{-2, -3}, {-3, -4}});

        // scalar multiplication
        REQUIRE(A*2 == Matrix{{2, 4}, {6, 8}});
        REQUIRE(B*3 == Matrix{{15, 18}, {21, 24}});
        REQUIRE(C*5 == Matrix{{10, 15}, {15, 20}});

        // scalar division
        REQUIRE(A/2 == Matrix{{1./2, 2./2}, {3./2, 4./2}});
        REQUIRE(B/3 == Matrix{{5./3, 6./3}, {7./3, 8./3}});
        REQUIRE(C/5 == Matrix{{2./5, 3./5}, {3./5, 4./5}});

        // matrix multiplication
        REQUIRE(A*B == Matrix{{19, 22}, {43, 50}});
        REQUIRE(A*C == Matrix{{8, 11}, {18, 25}});
        REQUIRE(B*C == Matrix{{28, 39}, {38, 53}});
    }

    SECTION("assignment operators") {
        A += C;
        B += C;
        REQUIRE(A == Matrix{{3, 5}, {6, 8}});
        REQUIRE(B == Matrix{{7, 9}, {10, 12}});

        A -= C;
        B -= C;
        REQUIRE(A == Matrix{{1, 2}, {3, 4}});
        REQUIRE(B == Matrix{{5, 6}, {7, 8}});

        A = C;
        B = C;
        C = Matrix{{0, 0}, {0, 0}};
        REQUIRE(A == Matrix{{2, 3}, {3, 4}});
        REQUIRE(B == Matrix{{2, 3}, {3, 4}});

        A = B.copy();
        C = A.copy();
        B = Matrix::identity(2);
        REQUIRE(A == Matrix{{2, 3}, {3, 4}});
        REQUIRE(C == A);
    }

    SECTION("multiplication") {
        // matrix multiplication
        B = Matrix{{1, 2, 3}, {2, 3, 4}};
        REQUIRE(A*B == Matrix{{5, 8, 11}, {11, 18, 25}});
        REQUIRE(C*B == Matrix{{8, 13, 18}, {11, 18, 25}});

        // vector multiplication
        Vector v = {1, 2};
        REQUIRE(A*v == Vector{5, 11});
        REQUIRE(C*v == Vector{8, 11});

        // transpose
        REQUIRE(A.T() == Matrix{{1, 3}, {2, 4}});
        REQUIRE(B.T() == Matrix{{1, 2}, {2, 3}, {3, 4}});
        REQUIRE(C.T() == Matrix{{2, 3}, {3, 4}});
    }

    SECTION("determinants") {
        A = {{4, 1}, {2, 3}};
        B = {{-2, 3, -1}, {5, -1, 4}, {4, -8, 2}};
        C = {{5, -7, 2, 2}, {0, 3, 0, -4}, {-5, -8, 0, 3}, {0, 5, 0, -6}};
        REQUIRE(A.det() == Approx(10));
        REQUIRE(B.det() == Approx(-6));
        REQUIRE(C.det() == Approx(20));
    }
}

TEST_CASE("Slices", "[math]") {
    SECTION("access") {
        Matrix A = {{1, 1, 2, 2}, {3, 3, 2, 2}, {5, 5, 4, 4}};
        
        // row through operator[]
        REQUIRE(A[0] == Vector{1, 1, 2, 2});
        REQUIRE(A[1] == Vector{3, 3, 2, 2});
        REQUIRE(A[2] == Vector{5, 5, 4, 4});

        // explicit row
        REQUIRE(A.row(0) == Vector{1, 1, 2, 2});
        REQUIRE(A.row(1) == Vector{3, 3, 2, 2});
        REQUIRE(A.row(2) == Vector{5, 5, 4, 4});

        // col
        REQUIRE(A.col(0) == Vector{1, 3, 5});
        REQUIRE(A.col(1) == Vector{1, 3, 5});
        REQUIRE(A.col(2) == Vector{2, 2, 4});
        REQUIRE(A.col(3) == Vector{2, 2, 4});
    }

    // assignment
    SECTION("assignment") {
        Matrix A = {{1, 1, 2, 2}, {3, 3, 2, 2}, {5, 5, 4, 4}};

        // row assignment
        A.row(1) = {9, 1, 2, 3};
        A.row(2) = {6, 3, 1, 2};
        REQUIRE(A == Matrix{{1, 1, 2, 2}, {9, 1, 2, 3}, {6, 3, 1, 2}});

        // column assignment
        A.col(1) = {2, 5, 1};
        A.col(3) = {7, 1, 3};
        REQUIRE(A == Matrix{{1, 2, 2, 7}, {9, 5, 2, 1}, {6, 1, 1, 3}});

        // minus-assignment
        A = {{1, 2, 2, 7}, {9, 5, 2, 1}, {6, 1, 1, 3}};
        A.row(0) -= A.row(1);
        A.row(1) -= A.row(2);
        REQUIRE(A.row(0) == Vector{-8, -3, 0, 6});
        REQUIRE(A.row(1) == Vector{3, 4, 1, -2});

        A = {{1, 2, 2, 7}, {9, 5, 2, 1}, {6, 1, 1, 3}};
        A.col(0) -= A.col(1);
        A.col(1) -= A.col(2);
        REQUIRE(A.col(0) == Vector{-1, 4, 5});
        REQUIRE(A.col(1) == Vector{0, 3, 0});

        // plus-assignment
        A = {{1, 2, 2, 7}, {9, 5, 2, 1}, {6, 1, 1, 3}};
        A.row(0) += A.row(1);
        A.row(1) += A.row(2);
        REQUIRE(A.row(0) == Vector{10, 7, 4, 8});
        REQUIRE(A.row(1) == Vector{15, 6, 3, 4});

        A = {{1, 2, 2, 7}, {9, 5, 2, 1}, {6, 1, 1, 3}};
        A.col(0) += A.col(1);
        A.col(1) += A.col(2);
        REQUIRE(A.col(0) == Vector{3, 14, 7});
        REQUIRE(A.col(1) == Vector{4, 7, 2});
    }

    SECTION("vector cast") {
        Matrix A = {{1, 2, 2, 7}, {9, 5, 2, 1}, {6, 1, 1, 3}};
        Vector a = A.col(2);
        REQUIRE((a.N == 3 && a.data.size() == 3));
        REQUIRE(a == Vector{2, 2, 1});
        REQUIRE((A.col(2).operator Vector().N == 3 && A.col(2).operator Vector().data.size() == 3)); // chain cast
    }

    SECTION("dot product") {
        Matrix A = {{1, 2, 2, 7}, {9, 5, 2, 1}, {6, 1, 1, 3}};

        // dot with vector
        Vector b = {2, 3, 1, 5};
        REQUIRE(A.row(0).dot(b) == (2+6+2+35));
        REQUIRE(A.row(2).dot(b) == (12+3+1+15));

        b = {1, 4, 2};
        REQUIRE(A.col(0).dot(b) == (1+36+12));
        REQUIRE(A.col(2).dot(b) == (2+8+2));

        // dot with other slice
        REQUIRE(A.col(0).dot(A.col(2)) == (2+18+6));
        REQUIRE(A.row(0).dot(A.row(1)) == (9+10+4+7));

    }

    SECTION("norm") {
        Matrix A = {{1, 2, 2, 7}, {9, 5, 2, 1}, {6, 1, 1, 3}};
        REQUIRE(A.col(0).norm() == sqrt(1+81+36));
        REQUIRE(A.row(0).norm() == sqrt(1+4+4+49));
    }
}

TEST_CASE("Cramer", "[math]") {
    Matrix A = {{2, 3}, {3, -4}};
    Vector b = {12, 1};
    Cramer2DSolver solver1(A);
    REQUIRE(solver1.solve(b) == Vector{3, 2});

    A = {{1, 2}, {4, 5}};
    b = {{3, 6}};
    Cramer2DSolver solver2(A);
    REQUIRE(solver2.solve(b) == Vector{-1, 2});

    A = {{2, -2}, {2, 2}};
    b = {{8, 2}};
    Cramer2DSolver solver3(A);
    REQUIRE(solver3.solve(b) == Vector{2.5, -1.5});

    // randomized tests on 2x2 matrices
    srand(time(NULL)); // seed rng
    for (int i = 0; i < 100; i++) {
        A = GenRandMatrix(2, 2);
        b = GenRandVector(2);
        Cramer2DSolver solver(A);
        Vector x = solver.solve(b);
        Vector Ax = A*x;
        REQUIRE(Ax == b);
    }
}

TEST_CASE("QRDecomposition", "[math]") {
    Matrix A = {{1, 2}, {3, 4}};
    QRDecomposition qr(A);
    REQUIRE(A*qr.inverse() == Matrix::identity(2));
    REQUIRE(qr.abs_determinant() == Approx(2));

    // randomized tests on 5x5 matrices
    srand(time(NULL)); // seed rng
    for (int i = 0; i < 10; i++) {
        A = GenRandMatrix(5, 5);
        Vector b = GenRandVector(5);
        QRDecomposition solver(A);
        Vector x = solver.solve(b);
        Vector Ax = A*x;
        REQUIRE(Ax == b);
        REQUIRE(solver.inverse()*A == Matrix::identity(5));
    }
}

TEST_CASE("Givens", "[broken],[math]") {
    Matrix A = {{2, 3}, {3, -4}};
    Vector b = {12, 1};
    GivensSolver solver1(A);
    REQUIRE(solver1.solve(b) == Vector{3, 2});

    A = {{1, 2}, {4, 5}};
    b = {{3, 6}};
    GivensSolver solver2(A);
    REQUIRE(solver2.solve(b) == Vector{-1, 2});

    A = {{2, -2}, {2, 2}};
    b = {{8, 2}};
    GivensSolver solver3(A);
    REQUIRE(solver3.solve(b) == Vector{2.5, -1.5});

    A = {{2, 3, 4}, {5, -6, 7}, {8, 9, 10}};
    b = {{119, 80, 353}};
    GivensSolver solver4(A);
    REQUIRE(solver4.solve(b) == Vector{12, 13, 14});

    // randomized tests on 5x5 matrices
    // srand(time(NULL)); // seed rng
    // for (int i = 0; i < 10; i++) {
    //     A = GenRandMatrix(5, 5);
    //     b = GenRandVector(5);
    //     GivensSolver solver(A);
    //     Vector x = solver.solve(b);
    //     Vector Ax = A*x;
    //     Ax.print();
    //     b.print();
    //     std::cout << std::endl;
    //     IS_TRUE(Ax == b);
    // }
}

TEST_CASE("cubic_spline", "[manual],[math]") {
    double b = 2*M_PI; // the range is from 0 to this
    int len = 10;
    double step = b/len;
    Vector x(len);
    Vector y(len);
    for (int i = 0; i < len; i++) {
        x[i] = i*step;
        y[i] = sin(x[i]);
    }
    double steps = 4; // interpolates 4 steps between points

    CubicSpline csin(x, y);
    std::vector<double> newx, newy;
    for (size_t i = 0; i < x.size()-1; i++) {
        newx.push_back(x[i]);
        newy.push_back(y[i]);
        for (double z = x[i]; z < x[i+1]; z += (x[i+1] - x[i])/steps) {
            newx.push_back(z);
            newy.push_back(csin.spline(z));
        }
    }
    std::unique_ptr<TCanvas> c = std::make_unique<TCanvas>("c1", "canvas", 600, 600);
    std::unique_ptr<TGraph> g1 = std::make_unique<TGraph>(newx.size(), &newx[0], &newy[0]);
    std::unique_ptr<TGraph> g2 = std::make_unique<TGraph>(len, &x.data[0], &y.data[0]);
    g1->SetLineColor(kRed);
    g1->Draw("AC");
    g2->Draw("SAME *");

    // setup the canvas and save the plot
    string path = "temp/cubicspline.pdf";
    c->SetRightMargin(0.15);
    c->SetLeftMargin(0.15);
    c->SaveAs(path.c_str());
}