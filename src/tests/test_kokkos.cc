#include <matpack.h>

#include <Kokkos_Core.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    constexpr std::size_t N = 1000000;

    Vector a(N, 1.0);

    using host_view_t = Kokkos::View<Numeric*,
                                     Kokkos::HostSpace,
                                     Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
    host_view_t a_host_view(a.data_handle(), a.size());

    using exec_space = Kokkos::DefaultExecutionSpace;
    using device_view_t =
        Kokkos::View<Numeric*, typename exec_space::memory_space>;
    device_view_t a_dev = Kokkos::create_mirror_view_and_copy(
        typename exec_space::memory_space(), a_host_view);

    // Initialize a
    Kokkos::parallel_for(
        "init",
        Kokkos::RangePolicy<exec_space>(0, (int)N),
        KOKKOS_LAMBDA(int i) { a_dev[i] = i; });

    // Compute the sum of
    Numeric sum = 0.0;
    Kokkos::parallel_reduce(
        "sum",
        Kokkos::RangePolicy<exec_space>(0, (int)N),
        KOKKOS_LAMBDA(int i, Numeric& lsum) { lsum += a_dev[i]; },
        sum);

    std::print("Sum Kokkos: {:.0f}\n", sum);
    std::print("Sum       : {:.0f}\n", matpack::sum(a));
  }
  Kokkos::finalize();
  return 0;
}
