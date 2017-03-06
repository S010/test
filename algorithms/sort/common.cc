#include <iostream>
#include <vector>

template<typename T>
std::istream& operator>>(std::istream& istr, std::vector<T>& v) {
  T elem;
  while (istr >> elem)
    v.push_back(elem);
  return istr;
}

template<typename T>
std::ostream& operator<<(std::ostream& ostr, const std::vector<T>& v) {
  for (auto i = begin(v); i != end(v); ++i) {
    ostr << *i;
    if (i != end(v) - 1)
      ostr << ' ';
  }
  return ostr;
}
