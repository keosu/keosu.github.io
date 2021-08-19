#include <iostream>
#include <tuple>
using namespace std;

//================ 非类型参数函数模板
template <long long N>
long long fact() {
  return fact<N - 1>() * N;  // 递归
}
template <>
long long fact<0>() {  // 特化，递归终止条件
  return 1;
}

// 二层循环
template <int i>
inline void IntBubbleSortLoop(int* data) {  // 一次冒泡，将前 i 个元素中最大的置换到最后
  IntBubbleSortLoop<i-1>(data);
  if(data[i-1] > data[i]) std::swap(data[i-1],data[i]);
}
template <>
inline void IntBubbleSortLoop<0>(int*) {}

template <int n>
inline void IntBubbleSort(int* data) {  // 模板冒泡排序循环展开
  IntBubbleSortLoop<n - 1>(data);
  IntBubbleSort<n - 1>(data);
}
template <>
inline void IntBubbleSort<1>(int* data) {}

//================ 非类型参数类模板
template <long long N>
struct Fact {
  static constexpr long long value = Fact<N - 1>::value * N;  // 递归
};
template <>
struct Fact<0> {
  static constexpr long long value = 1;  // 特化，递归终止条件
};

//================ 变长模板参数，sizeof 获取参数个数
template <typename... Types1, template <typename...> class T, typename... Types2,
          template <typename...> class V>
void bar(const T<Types1...>&, const V<Types2...>&) {
  std::cout << sizeof...(Types1) << std::endl;
  std::cout << sizeof...(Types2) << std::endl;
}

//================ 变长模板参数
// (1) 递归方式
template <typename T>
void write_line0(const T& t) {
  std::cout << t << '\n';
}
template <typename T, typename... Args>
void write_line0(const T& t, Args... args) {
  std::cout << t << ',';
  write_line0(args...);  //递归解决，利用模板推导机制，每次取出第一个，缩短参数包的大小。
}

// (2) 变长参数展开
template <typename... T>
void DummyWrapper(T... t) {}

template <class T>
T unpacker(const T& t) {
  cout << ',' << t;
  return t;
}
template <class T>
T* unpacker(T* t) {
  cout << ',' << t;
  return t;
}

template <typename T, typename... Args>
void write_line(const T& t, const Args&... data) {
  cout << t;
  (unpacker(data), ...);  // c++17 引入
  // DummyWrapper(unpacker(data)...);  //
  // c++17前所以需要用一个空函数包裹一下，但展开顺序与编译器对逗号表达式的求值顺序相关
  cout << '\n';
}

int main() {
  cout << "fact(5) = " << fact<5>() << endl;
  cout << "Fact(5) = " << Fact<5>::value << endl;

  std::tuple<int, double> a;
  std::tuple<char, float, long> b;
  bar(a, b);

  write_line0(1, 1.23);
  write_line0(1, 1.23, "abcde");
  write_line(1, 1.23);
  write_line(1, 1.23, "abcde");

  int inidata[4] = {3, 4, 2, 1};
  IntBubbleSort<4>(inidata);

  return 0;
}