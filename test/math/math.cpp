#include "plots/PlotOptions.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <math/Matrix.h>
#include <math/Vector.h>
#include <math/Vector3.h>
#include <math/Cramer2DSolver.h>
#include <math/CubicSpline.h>
#include <math/LUPDecomposition.h>
#include <math/QRDecomposition.h>
#include <math/Statistics.h>
#include <dataset/SimpleDataset.h>
#include <plots/PlotDataset.h>

#include <vector>
#include <iostream>

using std::cout, std::endl;

static double GenRandScalar() {
    return rand() % 100;
}

static Vector<double> GenRandVector(int m) {
    Vector<double> v(m);
    for (int i = 0; i < m; i++)
        v[i] = rand() % 100;
    return v;
}

static Matrix<double> GenRandMatrix(int n, int m) {
    Matrix<double> M(n, m);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            M[i][j] = rand() % 100;
    return M;
}

TEST_CASE("math: Cramer") {
    Matrix<double> A = {{2, 3}, {3, -4}};
    Vector<double> b = {12, 1};
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

TEST_CASE("math: QRDecomposition") {
    Matrix<double> A = {{1, 2}, {3, 4}};
    QRDecomposition qr(A);
    REQUIRE(A*qr.inverse() == matrix::identity(2));
    REQUIRE_THAT(qr.abs_determinant(), Catch::Matchers::WithinAbs(2, 1e-3));

    // randomized tests on 5x5 matrices
    srand(time(NULL)); // seed rng
    for (int i = 0; i < 10; i++) {
        A = GenRandMatrix(5, 5);
        Vector b = GenRandVector(5);
        QRDecomposition solver(A);
        Vector x = solver.solve(b);
        Vector Ax = A*x;
        REQUIRE(Ax == b);
        REQUIRE(solver.inverse()*A == matrix::identity(5));
    }
}

TEST_CASE("math: orthonormal_rotations") {
    for (int i = 0; i < 10; i++) {
        Vector3<double> angles = GenRandVector(3);
        Matrix R = matrix::rotation_matrix(angles.x(), angles.y(), angles.z());
        Matrix Ri = R.T();
        REQUIRE(R*Ri == matrix::identity(3));
    }

    for (int i = 0; i < 10; i++) {
        Vector3<double> axis = GenRandVector(3);
        double angle = GenRandScalar();
        Matrix R = matrix::rotation_matrix(axis, angle);
        Matrix Ri = R.T();
        REQUIRE(R*Ri == matrix::identity(3));
    }
}