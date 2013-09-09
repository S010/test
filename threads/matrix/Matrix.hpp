#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>

template<typename ValT>
class Matrix {
    public:
        Matrix():
            _height(1),
            _width(0)
        {
        }

        void push_back(ValT val) {
            _vec.push_back(val);
        }

        size_t width() const {
            return _width;
        }

        size_t height() const {
            return _height;
        }

        void width(size_t new_value) {
            if (new_value == 0 || _vec.size() % new_value != 0)
                throw std::runtime_error("Invalid width specified!");
            _width = new_value;
            _height = _vec.size() / _width;
        }

        void height(size_t new_value) {
            if (new_value == 0 || _vec.size() % new_value != 0)
                throw std::runtime_error("Invalid height specified!");
            _height = new_value;
            _width = _vec.size() / _height;
        }

        ValT& at(size_t row, size_t col) {
            return _vec[row * _width + col];
        }

        ValT at(size_t row, size_t col) const {
            return _vec[row * _width + col];
        }

        static Matrix load(const std::string& filepath) {
            Matrix<ValT> m;

            std::ifstream in(filepath.c_str());
            if (!in)
                throw std::runtime_error("Failed to open file!");

            std::string line;
            ValT val;
            while (std::getline(in, line)) {
                std::istringstream ss(line);
                size_t n_values = 0;
                while (ss >> val) {
                    m.push_back(val);
                    ++n_values;
                }
                m.width(n_values);
            }
            in.close();
            
            return m;
        }

    private:
        size_t _width;
        size_t _height;
        std::vector<ValT> _vec;
};

template<typename ValT>
std::ostream& operator<<(std::ostream& os, const Matrix<ValT>& m) {
    size_t row, col;

    for (row = 0; row < m.height(); ++row) {
        for (col = 0; col < m.width(); ++col) {
            if (col > 0)
                os << ' ';
            os << m.at(row, col);
        }
        os << std::endl;
    }

    return os;
}


