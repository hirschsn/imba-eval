// See LICENSE for license details.

#include <string>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <future>
#include <boost/program_options.hpp>

#include "mmapped_file.hpp"
#include "bin.hpp"
#include "dims_create.hpp"

bool divisible(ssize_t v, int d)
{
    return (v / d) * d == v;
}

using bins_type = Bins<3>;

bins_type bin_all(bins_type::index_type nbins, bins_type::point_type bounding_box, double const*first, double const*last)
{
    if (!divisible(last - first, 3)) {
        throw std::runtime_error("Range not a multiple of 3.");
    }

    auto b = bins_type{nbins, bounding_box};
    for (; first != last; first += 3)
        b.insert({first[0], first[1], first[2]});
    return b;
}

namespace statistics {
template <typename T, typename R = double>
R mean(const std::vector<T>& v)
{
    T sum = std::accumulate(v.begin(), v.end(), T{0}, std::plus<T>{});
    return static_cast<R>(sum) / v.size();
}

template <typename T, typename R = double>
R var(const std::vector<T>& v)
{
    T sqsum = std::accumulate(v.begin(), v.end(), T{0}, [](T acc, T val){ return acc + val * val; });
    R m = mean(v);
    return static_cast<R>(sqsum) / v.size() - m * m;
}
}

// For boost::program_options
namespace streamable {
template <int N>
struct NDoubles {
    typename Bins<N>::point_type data;
};
template <int N>
std::istream& operator>>(std::istream& is, NDoubles<N>& ti)
{
    char c;
    for (int i = 0; i < N; ++i) {
        is >> ti.data[i];
        if (i < N - 1)
            is.read(&c, 1);
    }
    return is;
}
}

int main(int argc, char **argv)
{
    const int nthreads = 4;
    using namespace std::string_literals;
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("file", po::value<std::string>(), "MPI-IO position file")
        ("box", po::value<streamable::NDoubles<3>>(), "Bounding box of the simulation")
        ("nproc", po::value<int>(), "Number of processes")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || !vm.count("file") || !vm.count("box") || !vm.count("nproc")) {
        std::cout << desc << std::endl;
        return 1;
    }

    const auto fn = vm["file"].as<std::string>();
    const auto nbins = dims_create(vm["nproc"].as<int>());
    const auto bbox = vm["box"].as<streamable::NDoubles<3>>().data;

    std::cout << "File : " << fn << std::endl;
    std::cout << "NProc: " << vm["nproc"].as<int>() << " = " << nbins[0] << " x " << nbins[1] << " x " << nbins[2] << std::endl;
    std::cout << "Box  : " << bbox[0] << " " << bbox[1] << " " << bbox[2] << "\n" << std::endl;

    auto f = MFile<double>{fn.c_str()};
    std::cout << "File has " << f.size() << " elemets." << std::endl;

    auto data = f.data();
    auto b = bin_all(nbins, bbox, data, data + f.size());

    /* Sanity check */
    int i = std::accumulate(b.bins.begin(), b.bins.end(), 0, std::plus<int>{});
    std::cout << "Binned   " << i << " particles." << std::endl;
    if (3 * i != f.size()) {
        throw std::runtime_error("Particles disappeared...");
    }
    /* End */

    std::cout << std::endl;
    std::cout << "Min: " << *std::min_element(b.bins.begin(), b.bins.end()) << std::endl;
    std::cout << "Max: " << *std::max_element(b.bins.begin(), b.bins.end()) << std::endl;

    auto dmean = statistics::mean(b.bins);
    auto dsdev = std::sqrt(statistics::var(b.bins));

    std::cout << "Mean: " << dmean << std::endl;
    std::cout << "SDev: " << dsdev << " ( = " << std::floor(dsdev / dmean * 1000.)/10. << " %)" << std::endl;
}