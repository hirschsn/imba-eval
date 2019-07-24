
#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

inline std::vector<int> prime_factors(int n) {
    std::vector<int> fs;
    // Preallocate some space (fully optional)
    fs.reserve(std::sqrt(static_cast<double>(n)));
    for (int p = 2; n > 1; ++p) {
        while (n % p == 0) {
            fs.push_back(p);
            n /= p;
        }
    }
    return fs;
}

inline std::array<int, 3> dims_create(int n) {
    auto fs = prime_factors(n);

    std::array<int, 3> dims = {{1, 1, 1}};
    // Greedy algorithm
    // Need to assign largest values first in order for the greedy algorithm
    // to work. No need to sort, the result of prime_factors() is sorted.
    std::for_each(std::rbegin(fs), std::rend(fs), [&dims](auto f) {
        *std::min_element(std::begin(dims), std::end(dims)) *= f;
    });

    return dims;
}
