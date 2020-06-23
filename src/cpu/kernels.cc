#include "cpu/kernels.h"

#if defined(__AVX2__)
#  define TARGET_ISA CpuIsa::AVX2
#  include "cpu/vec_avx.h"
#elif defined(__AVX__)
#  define TARGET_ISA CpuIsa::AVX
#  include "cpu/vec_avx.h"
#else
#  define TARGET_ISA CpuIsa::GENERIC
#  include "cpu/vec.h"
#endif

namespace ctranslate2 {
  namespace cpu {

    template <dim_t vec_width, typename Function>
    static void vectorized_iter(dim_t size, const Function& func) {
      const dim_t remaining = size % vec_width;
      size -= remaining;

      for (dim_t i = 0; i < size; i += vec_width) {
        func(i, vec_width);
      }

      if (remaining != 0) {
        func(size, remaining);
      }
    }

    template <CpuIsa ISA, typename T, typename Function>
    static void vectorized_unary_transform(const T* x, T* y, dim_t size, const Function& func) {
      vectorized_iter<Vec<T, ISA>::width>(size,
                                          [x, y, &func](dim_t i, dim_t width) {
                                            const auto v = Vec<T, ISA>::load(x + i, width);
                                            Vec<T, ISA>::store(func(v), y + i, width);
                                          });
    }

    template <CpuIsa ISA, typename T, typename Function>
    static void vectorized_binary_transform(const T* a,
                                            const T* b,
                                            T* c,
                                            dim_t size,
                                            const Function& func) {
      vectorized_iter<Vec<T, ISA>::width>(size,
                                          [a, b, c, &func](dim_t i, dim_t width) {
                                            const auto v1 = Vec<T, ISA>::load(a + i, width);
                                            const auto v2 = Vec<T, ISA>::load(b + i, width);
                                            Vec<T, ISA>::store(func(v1, v2), c + i, width);
                                          });
    }

    template <CpuIsa ISA, typename T>
    void rcp(const T* x, T* y, dim_t size) {
      vectorized_unary_transform<ISA>(x, y, size, Vec<T, ISA>::rcp);
    }

    template<>
    void exp<TARGET_ISA>(const float* x, float* y, dim_t size) {
      vectorized_unary_transform<TARGET_ISA>(x, y, size, Vec<float, TARGET_ISA>::exp);
    }

    template<>
    void log<TARGET_ISA>(const float* x, float* y, dim_t size) {
      vectorized_unary_transform<TARGET_ISA>(x, y, size, Vec<float, TARGET_ISA>::log);
    }
    template<>
    void sin<TARGET_ISA>(const float* x, float* y, dim_t size) {
      vectorized_unary_transform<TARGET_ISA>(x, y, size, Vec<float, TARGET_ISA>::sin);
    }

    template<>
    void cos<TARGET_ISA>(const float* x, float* y, dim_t size) {
      vectorized_unary_transform<TARGET_ISA>(x, y, size, Vec<float, TARGET_ISA>::cos);
    }

    template <CpuIsa ISA, typename T>
    void add(T a, const T* x, T* y, dim_t size) {
      const auto vec_a = Vec<T, ISA>::load(a);
      vectorized_unary_transform<ISA>(x, y, size,
                                      [vec_a](vec_type<T, ISA> v) {
                                        return Vec<T, ISA>::add(v, vec_a);
                                      });
    }

    template <CpuIsa ISA, typename T>
    void add(const T* a, const T* b, T* c, dim_t size) {
      vectorized_binary_transform<ISA>(a, b, c, size, Vec<T, ISA>::add);
    }

    template <CpuIsa ISA, typename T>
    void sub(const T* a, const T* b, T* c, dim_t size) {
      vectorized_binary_transform<ISA>(a, b, c, size, Vec<T, ISA>::sub);
    }

    template <CpuIsa ISA, typename T>
    void mul(T a, const T* x, T* y, dim_t size) {
      const auto vec_a = Vec<T, ISA>::load(a);
      vectorized_unary_transform<ISA>(x, y, size,
                                      [vec_a](vec_type<T, ISA> v) {
                                        return Vec<T, ISA>::mul(v, vec_a);
                                      });
    }

    template <CpuIsa ISA, typename T>
    void mul(const T* a, const T* b, T* c, dim_t size) {
      vectorized_binary_transform<ISA>(a, b, c, size, Vec<T, ISA>::mul);
    }

    template <CpuIsa ISA, typename T>
    void max(T a, const T* x, T* y, dim_t size) {
      const auto vec_a = Vec<T, ISA>::load(a);
      vectorized_unary_transform<ISA>(x, y, size,
                                      [vec_a](vec_type<T, ISA> v) {
                                        return Vec<T, ISA>::max(v, vec_a);
                                      });
    }

    template <CpuIsa ISA, typename T>
    void max(const T* a, const T* b, T* c, dim_t size) {
      vectorized_binary_transform<ISA>(a, b, c, size, Vec<T, ISA>::max);
    }

    template <CpuIsa ISA, typename T>
    void min(T a, const T* x, T* y, dim_t size) {
      const auto vec_a = Vec<T, ISA>::load(a);
      vectorized_unary_transform<ISA>(x, y, size,
                                      [vec_a](vec_type<T, ISA> v) {
                                        return Vec<T, ISA>::min(v, vec_a);
                                      });
    }

    template <CpuIsa ISA, typename T>
    void min(const T* a, const T* b, T* c, dim_t size) {
      vectorized_binary_transform<ISA>(a, b, c, size, Vec<T, ISA>::min);
    }

#define DECLARE_IMPL(T)                                                 \
    template void rcp<TARGET_ISA>(const T* x, T* y, dim_t size);        \
    template void add<TARGET_ISA>(T a, const T* x, T* y, dim_t size);   \
    template void add<TARGET_ISA>(const T* a, const T* b, T* c, dim_t size); \
    template void sub<TARGET_ISA>(const T* a, const T* b, T* c, dim_t size); \
    template void mul<TARGET_ISA>(T a, const T* x, T* y, dim_t size);   \
    template void mul<TARGET_ISA>(const T* a, const T* b, T* c, dim_t size); \
    template void max<TARGET_ISA>(T a, const T* x, T* y, dim_t size);   \
    template void max<TARGET_ISA>(const T* a, const T* b, T* c, dim_t size); \
    template void min<TARGET_ISA>(T a, const T* x, T* y, dim_t size);   \
    template void min<TARGET_ISA>(const T* a, const T* b, T* c, dim_t size);

    DECLARE_ALL_TYPES(DECLARE_IMPL)

  }
}
