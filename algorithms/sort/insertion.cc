#include <iostream>
#include <vector>

#include "common.cc"

template<typename T>
void Sort(std::vector<T>& A) {
  for (typename std::vector<T>::size_type j = 1; j < A.size(); ++j) {
    T key = A[j];
    auto i = j - 1;
    while (i > 1 && A[i] > key) {
      A[i + 1] = A[i];
      --i;
    }
    A[i + 1] = key;
  }
}

int main(int, char**) {
  std::vector<int> A;

  std::cin >> A;
  std::cout << "Before: " << A << std::endl;
  Sort(A);
  std::cout << "After: " << A << std::endl;

  return 0;
}
