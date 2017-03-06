#include <iostream>
#include <vector>

#include "common.cc"

template<typename T>
void MergeSort(T& v, typename T::size_type i, typename T::size_type j) {
  static unsigned count = 0;
  std::cout << "MergeSort call " << ++count << ", i=" << i << ", j=" << j << std::endl;

  auto k = i + (j - i) / 2;
  if (i < k && k < j) {
    MergeSort(v, i, k);
    MergeSort(v, k, j);
  }

  T A, B;

  std::cout << "copying into A: ";
  for (auto l = i; l < i; ++l) {
    std::cout << v[l] << ' ';
    A.push_back(v[l]);
  }
  std::cout << std::endl;

  std::cout << "copying into B: ";
  for (auto l = k; l < j; ++l) {
    std::cout << v[l] << ' ';
    B.push_back(v[l]);
  }
  std::cout << std::endl;

  std::cout << "merging A and B" << std::endl;
  k = i;
  for (i = j = 0; i < A.size() && j < B.size(); /* empty */) {
    if (A[i] < B[j]) {
      std::cout << "copying " << A[i] << " from A" << std::endl;
      v[k++] = A[i++];
    } else {
      std::cout << "copying " << B[j] << " from B" << std::endl;
      v[k++] = B[j++];
    }
  }

  while (i < A.size()) {
    std::cout << "copying " << A[i] << " from A" << std::endl;
    v[k++] = A[i++];
  }
  while (j < B.size()) {
    std::cout << "copying " << B[j] << " from B" << std::endl;
    v[k++] = B[j++];
  }
}

int main(int, char**) {
  std::vector<int> v;

  int i;
  while (std::cin >> i)
    v.push_back(i);

  std::cout << "Before: " << v << std::endl;
  MergeSort(v, 0, v.size());
  std::cout << "After: " << v << std::endl;

  return 0;
}
