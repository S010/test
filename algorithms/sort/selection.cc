#include <iostream>
#include <vector>

#include "common.cc"

void Sort(std::vector<int>& A) {
  for (size_t i = 0; i < A.size() - 1; ++i) {
    auto k = i;
    for (auto j = i + 1; j < A.size(); ++j) {
      if (A[j] < A[k])
        k = j;
    }
    if (k != i) {
      auto tmp = A[i];
      A[i] = A[k];
      A[k] = tmp;
    }
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
