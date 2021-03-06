/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 2.0
//              Copyright (2019) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Christian R. Trott (crtrott@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include "fill.hpp"

#include <experimental/mdspan>

#include <benchmark/benchmark.h>

#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <chrono>

//================================================================================

static constexpr int global_delta = 1;

template <class T, ptrdiff_t... Es>
using lmdspan = stdex::basic_mdspan<T, stdex::extents<Es...>, stdex::layout_left>;

//================================================================================

template <class MDSpan, class... DynSizes>
void BM_MDSpan_Stencil_3D(benchmark::State& state, MDSpan, DynSizes... dyn) {

  using value_type = typename MDSpan::value_type;
  auto buffer_size = MDSpan{nullptr, dyn...}.mapping().required_span_size();

  auto buffer_s = std::make_unique<value_type[]>(buffer_size);
  auto s = MDSpan{buffer_s.get(), dyn...};
  mdspan_benchmark::fill_random(s);

  auto buffer_o = std::make_unique<value_type[]>(buffer_size);
  auto o = MDSpan{buffer_o.get(), dyn...};
  mdspan_benchmark::fill_random(o);

  int d = global_delta;

  for (auto _ : state) {
    benchmark::DoNotOptimize(o);
    for(ptrdiff_t i = d; i < s.extent(0)-d; i ++) {
      for(ptrdiff_t j = d; j < s.extent(1)-d; j ++) {
        for(ptrdiff_t k = d; k < s.extent(2)-d; k ++) {
          value_type sum_local = 0;
          for(ptrdiff_t di = i-d; di < i+d+1; di++) { 
          for(ptrdiff_t dj = j-d; dj < j+d+1; dj++) { 
          for(ptrdiff_t dk = k-d; dk < k+d+1; dk++) { 
            sum_local += s(di, dj, dk);
          }}}
          o(i,j,k) = sum_local;
        }
      }
    }
    benchmark::ClobberMemory();
  }
  ptrdiff_t num_inner_elements = (s.extent(0)-d) * (s.extent(1)-d) * (s.extent(2)-d);
  ptrdiff_t stencil_num = (2*d+1) * (2*d+1) * (2*d+1);
  state.SetBytesProcessed( num_inner_elements * stencil_num * sizeof(value_type) * state.iterations());
}
MDSPAN_BENCHMARK_ALL_3D(BM_MDSpan_Stencil_3D, right_, stdex::mdspan, 80, 80, 80);
MDSPAN_BENCHMARK_ALL_3D(BM_MDSpan_Stencil_3D, left_, lmdspan, 80, 80, 80);
MDSPAN_BENCHMARK_ALL_3D(BM_MDSpan_Stencil_3D, right_, stdex::mdspan, 400, 400, 400);
MDSPAN_BENCHMARK_ALL_3D(BM_MDSpan_Stencil_3D, left_, lmdspan, 400, 400, 400);

//================================================================================

template <class T, class SizeX, class SizeY, class SizeZ>
void BM_Raw_Stencil_3D_right(benchmark::State& state, T, SizeX x, SizeY y, SizeZ z) {

  using MDSpan = stdex::mdspan<T, stdex::dynamic_extent, stdex::dynamic_extent, stdex::dynamic_extent>;  
  using value_type = typename MDSpan::value_type;
  auto buffer_size = MDSpan{nullptr, x,y,z}.mapping().required_span_size();

  T* s_ptr = nullptr;
  auto buffer_s = std::make_unique<value_type[]>(buffer_size);
  {
    auto s = MDSpan{buffer_s.get(), x, y, z};
    mdspan_benchmark::fill_random(s);
    s_ptr = s.data();
  }

  T* o_ptr = nullptr;
  auto buffer_o = std::make_unique<value_type[]>(buffer_size);
  {
    auto o = MDSpan{buffer_o.get(), x, y, z};
    mdspan_benchmark::fill_random(o);
    o_ptr = o.data();
  }

  int d = global_delta;

  for (auto _ : state) {
    benchmark::DoNotOptimize(o_ptr);
    for(ptrdiff_t i = d; i < x-d; i ++) {
      for(ptrdiff_t j = d; j < y-d; j ++) {
        for(ptrdiff_t k = d; k < z-d; k ++) {
          value_type sum_local = 0;
          for(ptrdiff_t di = i-d; di < i+d+1; di++) { 
          for(ptrdiff_t dj = j-d; dj < j+d+1; dj++) { 
          for(ptrdiff_t dk = k-d; dk < k+d+1; dk++) { 
            sum_local += s_ptr[dk + dj*z + di*z*y];
          }}}
          o_ptr[k + j*z + i*z*y] = sum_local;
        }
      }
    }
    benchmark::ClobberMemory();
  }
  ptrdiff_t num_inner_elements = (x-d) * (y-d) * (z-d);
  ptrdiff_t stencil_num = (2*d+1) * (2*d+1) * (2*d+1);
  state.SetBytesProcessed( num_inner_elements * stencil_num * sizeof(value_type) * state.iterations());
}
BENCHMARK_CAPTURE(BM_Raw_Stencil_3D_right, size_80_80_80, int(), ptrdiff_t(80), ptrdiff_t(80), ptrdiff_t(80));
BENCHMARK_CAPTURE(BM_Raw_Stencil_3D_right, size_400_400_400, int(), ptrdiff_t(400), ptrdiff_t(400), ptrdiff_t(400));

//================================================================================

BENCHMARK_MAIN();
