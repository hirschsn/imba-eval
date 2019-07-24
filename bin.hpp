// See LICENSE for license details.

#pragma once

#include <cmath>
#include <numeric>
#include <vector>
#include <array>

template <int D>
struct InlineFolder {
    using point_type = std::array<double, D>;

    point_type bbox;

    InlineFolder(point_type bbox): bbox(bbox) {}

    void operator()(point_type& p) {
        for (int i = 0; i < D; ++i)
            if (p[i] < 0.0 || p[i] >= bbox[i])
                p[i] -= std::floor(p[i] / bbox[i]) * bbox[i];
    }
};

template <int D>
struct Hasher {
    using index_type = std::array<int, D>;

    index_type nbins;

    Hasher(index_type nbins): nbins(nbins) {}

    size_t operator()(const index_type& idx) {
        return static_cast<size_t>((idx[2] * nbins[1] + idx[1]) * nbins[0] + idx[0]);
    }
};

template <typename T, size_t D>
inline T product(const std::array<T, D>& arr)
{
    return std::accumulate(std::begin(arr), std::end(arr), T{1}, std::multiplies<T>{});
}

template <int D>
struct Bins {
    using index_type = std::array<int, D>;
    using point_type = std::array<double, D>;
    index_type nbins;
    point_type bin_size, bounding_box;
    InlineFolder<D> fold;
    Hasher<D> hash;
    std::vector<size_t> bins;

    Bins(index_type nbins, point_type bounding_box): nbins(nbins), bounding_box(bounding_box), fold(bounding_box), hash(nbins), bins(product(nbins), size_t{0}) {
        for (int i = 0; i < D; ++i)
            bin_size[i] = bounding_box[i] / nbins[i];
    }

    void insert(point_type p) {
        std::array<int, D> bin;

        fold(p);
        for (int i = 0; i < D; ++i) {
            bin[i] = static_cast<int>(p[i] / bin_size[i]);
        }
        auto h = hash(bin);
        bins[h]++;
    }
};
