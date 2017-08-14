#include <unordered_set>
#include <vector>
#include <cstdint>

#include "benchmark/benchmark_api.h"
#include "GenerateInput.hpp"
#include "test_configs.h"
#include "test_utils.h"

#include<algorithm>
#include<chrono>
#include<deque>
#include<functional>
#include<list>

constexpr std::size_t TestNumInputs = 1024;

#define START_TIMER auto start = std::chrono::high_resolution_clock::now();
#define STOP_TIMER  auto end = std::chrono::high_resolution_clock::now();\
                    auto elapsed_seconds =\
                    std::chrono::duration_cast<std::chrono::duration<double>>(\
                    end - start);\
                    state.SetIterationTime(elapsed_seconds.count());


template <class GenInputs>
void BM_Sort(benchmark::State& st, GenInputs gen) {
    using ValueType = typename decltype(gen(0))::value_type;
    const auto in = gen(st.range(0));
    std::vector<ValueType> inputs[5];
    auto reset_inputs = [&]() {
        for (auto& C : inputs) {
            C = in;
            benchmark::DoNotOptimize(C.data());
        }
    };
    reset_inputs();
    while (st.KeepRunning()) {
        for (auto& I : inputs) {
            std::sort(I.data(), I.data() + I.size());
            benchmark::DoNotOptimize(I.data());
        }
        st.PauseTiming();
        reset_inputs();
        benchmark::ClobberMemory();
        st.ResumeTiming();
    }
}

template<typename V>
void BM_sort_std_common(benchmark::State& state) {
  int N = state.range(0);
  V v(N);
  fill_random(v);
  using T = typename V::value_type;
  while (state.KeepRunning()) {
    std::vector<T> vec(v.begin(), v.end());
    START_TIMER
    std::sort(vec.begin(), vec.end());
    STOP_TIMER
  }
  state.SetComplexityN(N);
}

template<typename V>
void BM_sort_std_list_with_vector(benchmark::State& state) {
  int N = state.range(0);
  V v(N);
  fill_random(v);
  using T = typename V::value_type;
  // Copy the contents into a vector
  while (state.KeepRunning()) {
    std::vector<T> vec(v.begin(), v.end());
    // Sort the vector
    std::sort(vec.begin(), vec.end());
    // Put the item back in the list
    v.assign(vec.begin(), vec.end());
  }
  state.SetComplexityN(N);
}

// Sort (a sequence in ascending order) in ascending order.
template<typename V>
void BM_sort_std_ascending(benchmark::State& state) {
  int N = state.range(0);
  using T = typename V::value_type;
  V v(N);
  fill_seq(v);
  while (state.KeepRunning()) {
    std::vector<T> vec(v.begin(), v.end());
    START_TIMER
    std::sort(vec.begin(), vec.end(), std::less<T>());
    STOP_TIMER
  }
  state.SetComplexityN(N);
}

// Sort (a sequence in ascending order) in descending order.
template<typename V>
void BM_sort_std_descending(benchmark::State& state) {
  int N = state.range(0);
  using T = typename V::value_type;
  V v(N);
  fill_seq(v);
  while (state.KeepRunning()) {
    std::vector<T> vec(v.begin(), v.end());
    START_TIMER
    std::sort(vec.begin(), vec.end(), std::greater<T>());
    STOP_TIMER
  }
  state.SetComplexityN(N);
}

template<typename V>
void BM_sort_std_worst_quick(benchmark::State& state) {
  int N = state.range(0);
  using T = typename V::value_type;
  V v;
  make_killer(N, v);
  while (state.KeepRunning()) {
    std::vector<T> vec(v.begin(), v.end());
    START_TIMER
    std::sort(vec.begin(), vec.end());
    STOP_TIMER
  }
  state.SetComplexityN(N);
}




BENCHMARK_CAPTURE(BM_Sort, random_uint32,
    getRandomIntegerInputs<uint32_t>)->Arg(TestNumInputs);

BENCHMARK_CAPTURE(BM_Sort, sorted_ascending_uint32,
    getSortedIntegerInputs<uint32_t>)->Arg(TestNumInputs);

BENCHMARK_CAPTURE(BM_Sort, sorted_descending_uint32,
    getReverseSortedIntegerInputs<uint32_t>)->Arg(TestNumInputs);

BENCHMARK_CAPTURE(BM_Sort, single_element_uint32,
    getDuplicateIntegerInputs<uint32_t>)->Arg(TestNumInputs);

BENCHMARK_CAPTURE(BM_Sort, pipe_organ_uint32,
    getPipeOrganIntegerInputs<uint32_t>)->Arg(TestNumInputs);

BENCHMARK_CAPTURE(BM_Sort, random_strings,
    getRandomStringInputs)->Arg(TestNumInputs);

BENCHMARK_CAPTURE(BM_Sort, sorted_ascending_strings,
    getSortedStringInputs)->Arg(TestNumInputs);

BENCHMARK_CAPTURE(BM_Sort, sorted_descending_strings,
    getReverseSortedStringInputs)->Arg(TestNumInputs);

BENCHMARK_CAPTURE(BM_Sort, single_element_strings,
    getDuplicateStringInputs)->Arg(TestNumInputs);

#define COMPLEXITY_BENCHMARK_GEN_T(T) \
    COMPLEXITY_BENCHMARK_GEN(BM_sort_std_common, std::vector<T>, MSize);\
    COMPLEXITY_BENCHMARK_GEN(BM_sort_std_ascending, std::vector<T>, MSize);\
    COMPLEXITY_BENCHMARK_GEN(BM_sort_std_descending, std::vector<T>, MSize);\

COMPLEXITY_BENCHMARK_GEN_T(int)
//COMPLEXITY_BENCHMARK_GEN_T(double)
COMPLEXITY_BENCHMARK_GEN_T(aggregate)

COMPLEXITY_BENCHMARK_GEN(BM_sort_std_list_with_vector, std::list<int>, MSize);
COMPLEXITY_BENCHMARK_GEN(BM_sort_std_list_with_vector, std::list<aggregate>, MSize);
COMPLEXITY_BENCHMARK_GEN(BM_sort_std_worst_quick, std::vector<int>, MSize);

BENCHMARK_MAIN()
