#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gmp.h>

struct IsPrimeStruct {
  mpz_t root;
  mpz_t rem;
  mpz_t i;

  IsPrimeStruct() {
    mpz_init(root);
    mpz_init(rem);
    mpz_init(i);
  }

  ~IsPrimeStruct() {
    mpz_clear(root);
    mpz_clear(rem);
    mpz_clear(i);
  }

  bool IsPrime(const mpz_t& n) {
    int prob;

    prob = mpz_probab_prime_p(n, 15);
    if (prob == 2)
      return true;
    else if (prob == 0)
      return false;

    mpz_root(root, n, 2);

    // check if it's a square number
    mpz_mod(rem, n, root);
    if (mpz_cmp_ui(rem, 0) == 0)
      return false;

    // check if it's a multiple of the first few primes
    for (auto x: { 2, 3, 5, 7, 11 }) {
      mpz_set_ui(i, x);
      mpz_mod(rem, n, root);
      if (mpz_cmp_ui(rem, 0) == 0)
        return false;
    }

    mpz_set_ui(i, 13);
    while (mpz_cmp(i, root) <= 0) {
      mpz_mod(rem, n, i);
      if (mpz_cmp_ui(rem, 0) == 0)
        return false;
      mpz_add_ui(i, i, 2);
    }

    return true;
  }
};

int main(int argc, char** argv)
{
  unsigned i;
  mpz_t n;
  char *s;
  size_t len;

  mpz_init(n);

  for (i = 2; i < 20; i++) {
    mpz_ui_pow_ui(n, 2*i - 1, 2*i - 1);
    mpz_sub_ui(n, n, 2);
    s = mpz_get_str(NULL, 10, n);
    len = strlen(s);
    std::cout << i << " (" << len << " digits): " << std::flush;
    std::cout << std::boolalpha << IsPrimeStruct{}.IsPrime(n) << std::endl;
  }
}
