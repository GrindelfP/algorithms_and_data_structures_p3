## САОД. Задание 9: Многопоточность

### Постановка задачи

Необходимо реализовать многопоточную обработку массива чисел и сравнить производительность различных подходов синхронизации потоков.

Для каждого элемента массива вызывается вычислительно тяжёлая функция `special()`, основанная на рекурсивном вычислении чисел Фибоначчи. Она вычисляет четность суммы первых `n` чисел Фибоначчи.

Задача заключается в подсчёте числа элементов, для которых сумма первых `n` чисел Фибоначчи является чётной.

Для числа 5 будет вычислено выражение: `0 + 1 + 1 + 2 + 3 = 7 % 2 = 1`.

Для числа 9 будет вычислено выражение: `0 + 1 + 1 + 2 + 3 + 5 + 8 + 13 + 21 = 54 % 2 = 0`.

### Синтетический тест

```
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <random>
#include <vector>
#include <functional>

typedef unsigned long long int bigint; // at least 64 bits

bigint fibonacci(bigint n)
{
  return n < 2 ? n : fibonacci(n - 1) + fibonacci(n - 2);
}

bool special(int n)
{
  bigint sum = 0;
  for(int i = 0; i != n; i++)
    sum += fibonacci(i);
  return sum % 2 == 0;
}

size_t single(const std::vector<int>& v)
{
  return std::count_if(v.begin(), v.end(), [](const auto &el)
  {
    return special(el);
  });
}

size_t block(const std::vector<int>& v, size_t n_threads)
{
  std::vector<size_t> results(n_threads, 0);
  auto lambda = [&v, &results](size_t a, size_t b, size_t thread_id)
  {
    auto sum = std::count_if(v.begin() + a, v.begin() + b, [](const auto &el)
    {
      return special(el);
    });
    results[thread_id] = sum;
  };

  std::vector<std::thread> threads(n_threads);
  size_t part_size = v.size() / n_threads, a = 0, b = 0;
  for(size_t t = 0; t != n_threads; t++, a = b)
  {
    b = (t == n_threads - 1) ? v.size() : a + part_size;
    threads[t] = std::thread(lambda, a, b, t);
  }

  for(auto& t : threads)
    t.join();

  return std::accumulate(results.begin(), results.end(), 0ULL);
}

size_t this_is_the_way(const std::vector<int>& v, size_t n_threads)
{
  return 0;
}

size_t is_this_the_way(const std::vector<int>& v, size_t n_threads)
{
  return 0;
}

// g++ -std=c++20 -O3 main.cpp
// clang++ -Wall -stdlib=libc++ -std=c++20 -O3 main.cpp
int main()
{
  std::vector<int> v(12); // для реального теста используйте размер >= 50 элементов

  std::mt19937_64 gen;
  gen.seed(1);
  std::poisson_distribution<> pd(4);

  std::cout << "# ";
  for(auto & item : v)
  {
    int value = 40 + pd(gen);
    item = value < 53 ? value : 53;
    std::cout << item << ',';
  }
  std::cout << std::endl;

  //std::sort(v.begin(), v.end());

  std::vector<std::function<size_t(const std::vector<int>& v, size_t n_threads)>> functions =
  {
    block, this_is_the_way, is_this_the_way
  };
  std::cout << "#nthreads single block" << std::endl;

  auto st1 = std::chrono::high_resolution_clock::now();
  auto nsingle = single(v);
  auto st2 = std::chrono::high_resolution_clock::now();

  auto single_time = std::chrono::duration_cast<std::chrono::milliseconds>(st2 - st1).count();
  std::cout << 1 << '\t';
  for(size_t i = 0; i < functions.size() + 1; i++)
    std::cout << single_time  << '\t';
  std::cout << nsingle << std::endl;

  for(size_t th_number : {2, 3, 4})
  {
    std::vector<size_t> number(functions.size());
    std::cout << th_number << '\t' << single_time << '\t';
    for(size_t j = 0; auto &f : functions)
    {
      const auto t1{std::chrono::high_resolution_clock::now()};
      number[j++] = f(v, th_number);
      const auto t2{std::chrono::high_resolution_clock::now()};
      const auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

      std::cout << exectime << '\t';
    }
    for(const auto & item : number)
      std::cout << item << '\t';
    std::cout << std::endl;
  }
}
```

## Варианты заданий

C11: pthreads + mutex, stdatomic, semaphore, etc

OpenMP: reduction, dynamic scheduling, etc

C++: mutex, atomic, semaphore, tbb, etc

C#: Thread, ThreadPool, TPL, etc

Kotlin: Coroutines, Channels, etc

Python: threading, multiprocessing, etc

### Требования (для всех вариантов)

Необходимо:

1. Измерить время выполнения.
2. Посчитать ускорение: `speedup = T_single / T_parallel`
3. Посчитать эффективность: `efficiency = speedup / N_threads`

### Отчёт

Отчёт должен содержать:

1. Исходный код.
2. Таблицу времени выполнения для вашей вычислительной системы.
3. График ускорения.
4. График эффективности.

### Критерии оценки

50 баллов:

- Реализован один подход многопоточности помимо блочной реализации.
- Код компилируется.
- Программа работает корректно.
- Потоки завершаются без ошибок.
- Получены корректные результаты вычислений.

100 баллов:

- Реализовано несколько подходов многопоточности помимо блочной реализации.
- Выполнено сравнение различных реализаций.
- Построены графики: ускорения (speedup), эффективности (efficiency).
