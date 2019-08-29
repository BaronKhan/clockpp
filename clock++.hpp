/** clock++.hpp
 *
 * Clock++: A minimal GNU C++11 performance measurement header file
 * Author: Baron Khan
 *
 * Description:
 * - Features the simplest interface for timing code, in a single header file
 *   - Measure the execution time between two points in any function body
 *   - Measure the time taken for any function to execute with arguments
 * - Fully compatible with both GCC and Clang C++11 compilers
 * - Timing accuracy is within 0.1 microsecond margin
 * - Open-source, high performance and thread-safe
 *
 * Installation (Unix):
 *   $ sudo cp clock++.hpp /usr/include/.
 *
 * Usage:
 *   #include <clock++.hpp>
 *
 *   CLOCK_START();
 *   ...
 *   CLOCK_STOP();
 *   CLOCK_START();
 *   auto time_ns=CLOCK_STOP();
 *
 * Output to STDERR:
 *   [CLOCK] main.cpp::int main() lines 20-22 [thread 7f797ecb0740]: 1915000 ns
 *   [CLOCK] main.cpp::int main() lines 23-24 [thread 7f797ecb0740]: 1300 ns
 *
 * Measuring Functions:
 *   const auto time_ns=CLOCK(foo);               // foo();
 *   CLOCK(bar, 10, "Hello World!");              // bar(10, "Hello World!");
 *   CLOCK([&](bool x) -> int { ... }, false);    // Lambda functions
 *
 * Output for Functions:
 *   [CLOCK] source.cpp::foo()    line 31 [thread 7faab97c0740]:      631900 ns
 *   [CLOCK] source.cpp::bar()    line 32 [thread 7f26f7e00740]:      554900 ns
 *   [CLOCK] source.cpp::lambda() line 33 [thread 7faab97c0740]:      578100 ns
 *
 *
 * Licensed under the MIT License. Contributions are encouraged.
 *
 * Future work:
 * - Support for MinGW, Visual Studio and Emscripten compilers
 * - Allow for repeated measurements, parallelized using OpenMP
 */

#ifndef CLOCKPLUSPLUS_HPP
#define CLOCKPLUSPLUS_HPP

#if __cplusplus <= 199711L
  #error This library needs at least a C++11 compliant compiler
#endif


#define CLOCK_START() \
  clockpp::clock_start(__FILE__, __PRETTY_FUNCTION__, __LINE__)

#define CLOCK_STOP() \
  clockpp::clock_stop(__FILE__, __PRETTY_FUNCTION__, __LINE__)

#define CLOCK(f, ...) \
  clockpp::clock_func(f, __FILE__, __LINE__, #f "()", ##__VA_ARGS__)

#include <chrono>
#include <cstdio>
#include <functional>
#include <unordered_map>
#include <cstring>
#include <stack>

#ifdef _PTHREAD_H
#include <sys/types.h>
#endif

#ifdef __clang__
#include <iostream>
#endif

namespace clockpp
{

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
typedef std::chrono::nanoseconds Ns;

struct alignas(64) Timer
{
  TimePoint m_time;
  int m_line;
};

static std::unordered_map<std::string,
  std::unordered_map<long int, std::stack<Timer>>> s_start_times;

inline std::string get_location(const char *file, const char *function)
{
  char buff[128];
  snprintf(buff, 128, "%s::%s", file, function);
  return buff;
}

inline long int get_thread_id()
{
#ifdef _PTHREAD_H
  return pthread_self();
#else
  return 0;
#endif
}

inline void clock_start(const char *file, const char *function, int line)
{
  const auto thread_id = get_thread_id();
  const auto location = get_location(file, function);

  auto & start_times = s_start_times[location][thread_id];
  start_times.emplace(Timer{TimePoint{}, line});
  start_times.top().m_time = Clock::now();
}

inline unsigned long long clock_stop(const char *file, const char *function, 
                                     int line)
{
  const auto end = Clock::now();
  const auto thread_id = get_thread_id();
  const auto location = get_location(file, function);
  auto & start_times = s_start_times[location][thread_id];

  if (start_times.empty())
    return 0llu;

  auto & start = start_times.top();
  const auto diff = std::chrono::duration_cast<Ns>(end-start.m_time).count();

  fprintf(stderr, "[CLOCK]\t%s\tlines %u-%u [thread %lx]:\t%lu ns\n",
          location.c_str(), start.m_line, line, thread_id, diff);

  start_times.pop();

  return diff;
}

template<class C, class ...P>
unsigned long long clock_func(C callable, const char *file, int line,
                              const char *f_name, P... params)
{
  const bool is_lambda = strchr(f_name, '[');
  const auto thread_id = get_thread_id();
  const auto location = get_location(file, is_lambda ? "lambda()" : f_name);

  std::function<void()> func = std::bind(callable, params...);
  const auto start = Clock::now();
  func();
  const auto end = Clock::now();
  const auto diff = std::chrono::duration_cast<Ns>(end-start).count();

  fprintf(stderr, "[CLOCK]\t%s\tline %u [thread %lx]:\t%lu ns\n",
          location.c_str(), line, thread_id, diff);

  return diff;
}

} /* clockpp */

#endif /* CLOCKPLUSPLUS_HPP */

/*** end of file ***/