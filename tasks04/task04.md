## САОД. Задание 4: Поиск подстрок.

### Постановка задачи

Дан большой ASCII-файл (дамп статей из Википедии), который рассматривается как одна длинная строка и загружается в строковый объект.

Необходимо реализовать следующие алгоритмы поиска подстроки:
- наивный алгоритм (для сравнения),
- алгоритм Кнута-Морриса-Пратта (КМП),
- алгоритм Бойера-Мура (БМ).

### Требования

1. Реализовать все три алгоритма.
2. Для каждого алгоритма:
   - выполнить не менее 25 запусков поиска одной и той же подстроки,
   - измерить время выполнения каждого запуска.
3. Рассчитать:
   - среднее время поиска,
   - разброс значений (например, минимум, максимум, медиану).

Подстрока указана в примере.

```cpp
#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <fstream>
#include <vector>

size_t naive(std::string_view str, std::string_view sub)
{
 size_t n = str.size(), m = sub.size(), j = 0;
 for(size_t i = 0; i < n - m + 1; i++)
 {
  for(j = 0; j < m; j++)
  {
   if(str[i + j] != sub[j])
    break;
  }
  if(j == m)
   return i;
 }
 return std::string::npos;
}

size_t kmp(std::string_view str, std::string_view sub)
{
 // ваша быстрая реализация KMP
 return std::string::npos;
}

size_t bm(std::string_view str, std::string_view sub)
{
 // ваша быстрая реализация BM
 return std::string::npos;
}

int main()
{
 using namespace std;

 string str, sub = "and is the second single from their greatest hits";
 ifstream fin("simplewiki-20260201.txt", ios::binary);
 if(!fin.is_open())
 {
  cout << "not open!" << endl;
  return 0;
 }
 str.append((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());

 size_t n = 10;
 std::vector<size_t> times(n), indx(n);

 cout << "\nstd::find\n";
 for(size_t i = 0; i < n; i++)
 {
  auto time_one = chrono::high_resolution_clock::now();
  auto index = str.find(sub);
  if(index == std::string::npos)
   std::cout << "not found\n";
  else
   indx[i] = index;
  auto time_two = chrono::high_resolution_clock::now();
  times[i] = chrono::duration_cast<chrono::microseconds>(time_two - time_one).count();
 }
 for(size_t i = 0; i < n; i++)
 {
  cout << indx[i] << '\t' << times[i] << endl;
 }

 cout << "\nnaive\n";
 for(size_t i = 0; i < n; i++)
 {
  auto time_one = chrono::high_resolution_clock::now();
  auto index = naive(str, sub);
  if(index == std::string::npos)
   std::cout << "not found\n";
  else
   indx[i] = index;
  auto time_two = chrono::high_resolution_clock::now();
  times[i] = chrono::duration_cast<chrono::microseconds>(time_two - time_one).count();
 }
 for(size_t i = 0; i < n; i++)
 {
  cout << indx[i] << '\t' << times[i] << endl;
 }
}

```
### Отчёт

В отчёте необходимо представить:

- диаграммы размаха (boxplot) времени выполнения для каждого алгоритма;
- таблицу со средними значениями времени.

### Критерии оценки

Оценка зависит от корректности реализации и производительности алгоритмов:

- 50 баллов - если среднее время поиска алгоритмом КМП больше среднего времени наивного алгоритма;
- 100 баллов - если среднее время поиска алгоритмом КМП меньше среднего времени наивного алгоритма.

