#include <iostream>
#include <string>

#include "Matrix.hpp"

template<typename ValT>
average(const Matrix<ValT>& m) {
    size_t row, col;
    for (row = 0; row < m.height(); ++row) {
        for (col = 0; col < m.width(); ++col) {
        }
    }
}

int main(int argc, char** argv) {
    while (++argv, --argc) {
        Matrix<long> m(Matrix<long>::load(*argv));
        std::cout << m << std::endl;
    }
    return 0;
}
