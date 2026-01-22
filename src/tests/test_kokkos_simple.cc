#include <Kokkos_Core.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    constexpr std::size_t N = 100'000'000;

    std::vector<double> a;
    a.resize(N);

    Kokkos::Timer timer;
    double timer_start = timer.seconds();
    double timer_stop{};

    using host_view_t = Kokkos::View<double*,
                                     Kokkos::HostSpace,
                                     Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
    host_view_t a_host_view(a.data(), a.size());

    using exec_space = Kokkos::HostSpace::execution_space;
    using device_view_t =
        Kokkos::View<double*, typename exec_space::memory_space>;
    device_view_t a_dev = Kokkos::create_mirror_view_and_copy(
        typename exec_space::memory_space(), a_host_view);

    // Initialize a
    Kokkos::parallel_for(
        "init",
        Kokkos::RangePolicy<exec_space>(0, (int)N),
        KOKKOS_LAMBDA(int i) {
          a_dev[i] = 1.0;
        });
    
    Kokkos::fence();

    // Compute the sum of
    double sum = 0.0;
    Kokkos::parallel_reduce(
        "sum",
        Kokkos::RangePolicy<exec_space>(0, (int)N),
        KOKKOS_LAMBDA(int i, double& lsum) { lsum += a_dev[i]; },
        sum);

    Kokkos::fence();

    timer_stop = timer.seconds();

    std::cout << "Time taken Kokkos: " << timer_stop - timer_start << std::endl;

    timer_start = timer.seconds();
    double sum2{};
    #pragma omp parallel for
    for (std::size_t i = 0; i < N; i++) a[i] = 1.;

    #pragma omp parallel for reduction(+:sum2)
    for (std::size_t i = 0; i < N; i++) sum2 += a[i];

    timer_stop = timer.seconds();
    std::cout << "Time taken       : " << timer_stop - timer_start << std::endl;
    std::cout << "Sum Kokkos: " << sum << "\n";
    std::cout << "Sum       : " << sum << "\n";
  }
  Kokkos::finalize();
  return 0;
}
