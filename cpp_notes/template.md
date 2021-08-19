# C++模板简介
C++模板是为了实现泛型编程(Generic Programming)，是指数据结构和算法的设计不依赖于特定的类型。 
模板主要分为函数模板和类模板    
模板的参数可以是：
- 类型参数,用typename或class标记
- 非类型参数，整数或者枚举类型，外部链接对象的指针
- 模板型参数，如 ·template<typename T, template<typename> class A> someclass {};·

## 函数模板
```cpp
template<typename T>
T max(T a, T b) {
  return (a>b)?a:b;
}
void test() {
  double a = 1.23, b = 4.23;
  float c = 3.45, d = 5.45;
  int e = 123, f = 456;
  auto aa = max(a, b);
  auto bb = max(c, d);
  auto cc = max(e, f);
}
```

## 类模板
```cpp
template<typename T>
class A {
public:
  T t;
};
```

## 非类型模板参数
非类型参数模板，指模板参数不限于类型
非类型模板参数是有类型限制的。一般而言，它可以是常整数（包括enum枚举类型）或者指向外部链接对象的指针。
这样，通过设定终止条件，可以实现编译期的数值计算。下面是一个单层循环（计算阶乘）和两层循环的例子（冒泡排序）
```cpp
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
inline void IntBubbleSortLoop(int* data) {  // 一次冒泡，将最大的置换到最后
  IntBubbleSortLoop<i-1>(data);
  if(data[i-1] > data[i]) std::swap(data[i-1],data[i]);
}
template <>
inline void IntBubbleSortLoop<0>(int*) {} // 特化，递归终止条件

template <int n>
inline void IntBubbleSort(int* data) {  // 模板冒泡排序循环展开
  IntBubbleSortLoop<n - 1>(data);
  IntBubbleSort<n - 1>(data);
}
template <>
inline void IntBubbleSort<1>(int* data) {} // 特化，递归终止条件
```

## 变长模板参数
C++11后引入了对变长模板的支持,下面是一个简单的打印例子  
```cpp
template <typename T>
void write_line0(const T& t) {
  std::cout << t << '\n';
}
template <typename T, typename... Args>
void write_line0(const T& t, Args... args) {
  std::cout << t << ',';
  write_line0(args...);  //递归解决，利用模板推导机制，每次取出第一个，缩短参数包的大小。
}
```
上面是通过递归的方式，每次打印一个参数，也可以通过参数展开,C++17之后可以利用折叠表达式特性  
```cpp
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
```

# 模板特化

模板特化(template specialization)：对于通例中的某种或某些情况做单独专门实现
- 完全特例化（full specialization）:每个模板参数指定一个具体值
- 部分特例化、偏特化（partial specialization）：部分参数指定一个具体值

模板实例化（template instantiation）：编译时根据实参替换模板参数
- 隐式实例化（implicit instantiation）：使用模板时自动地实例化代码，模板的成员函数一直到引用时才被实例化；
- 显式实例化（explicit instantiation）：直接声明模板实例化，模板所有成员立即都被实例化；

**注意**   
> 隐式实例化时，成员只有被引用到才会进行实例化，即推迟实例化（lazy instantiation），这可能为隐藏一些问题。
> 因此可以在测试时，显示调用，及时发现错误。  
> 实例化时，优先匹配特化版本，若有多个特化版本匹配，且不为子集，则报错

函数名称查找顺序：
- 先找参数完全匹配的普通函数（非由模板实例化得到的函数）。
- 再找参数完全匹配的模板函数。
- 再找实参经过自动类型转换后能够匹配的普通函数。
- 如果上面的都找不到，则报错。

```cpp
template<typename T, int i> class cp00; // 用于模板型模板参数
// 通例
template<typename T1, typename T2, int i, template<typename, int> class CP>
class TMP;
// 完全特例化
template<>
class TMP<int, float, 2, cp00>;
// 第一个参数有const修饰
template<typename T1, typename T2, int i, template<typename, int> class CP>
class TMP<const T1, T2, i, CP>;
// 第一二个参数为cp00的实例且满足一定关系，第四个参数为cp00
template<typename T, int i>
class TMP<cp00<T, i>, cp00<T, i+10>, i, cp00>;
// 编译错误!，第四个参数类型和通例类型不一致
//template<template<int i> CP>
//class TMP<int, float, 10, CP>;
```
# 策略和特性
policy类和trait（或者称为trait模板）是两种C++程序设计机制，它们有助于对某些额外参数的管理，这里的额外参数是指：在具有工业强度的模板设计中所出现的参数   
特性对类型的信息（如 value_type、 reference）进行包装，使得上层代码可以以统一的接口访问这些信息。C++ 模板元编程会涉及大量的类型计算，很多时候要提取类型的信息（typedef、 常量值等），如果这些类型的信息的访问方式不一致（如上面的迭代器和指针），我们将不得不定义特例，这会导致大量重复代码的出现（另一种代码膨胀），而通过加一层特性可以很好的解决这一问题。另外，特性不仅可以对类型的信息进行包装，还可以提供更多信息，当然，因为加了一层，也带来复杂性。特性是一种提供元信息的手段。

策略（policy）一般是一个类模板，典型的策略是 STL 容器（如 std::vector<>，完整声明是template<class T, class Alloc=allocator<T>> class vector;）的分配器（这个参数有默认参数，即默认存储策略），策略类将模板的经常变化的那一部分子功能块集中起来作为模板参数，这样模板便可以更为通用，这和特性的思想是类似的（详见文献[1] 12.3）。

标签（tag）一般是一个空类，其作用是作为一个独一无二的类型名字用于标记一些东西，典型的例子是 STL 迭代器的五种类型的名字（input_iterator_tag, output_iterator_tag, forward_iterator_tag, bidirectional_iterator_tag, random_access_iterator_tag），std::vector<int>::iterator::iterator_category 就是 random_access_iterator_tag，可以用第1节判断类型是否等价的模板检测这一点：

```cpp
// 特性，traits
template<typename iter>
class mytraits{
public: typedef typename iter::value_type value_type;
};
template<typename T>
class mytraits<T*>{
public: typedef T value_type;
};

template<typename iter>
typename mytraits<iter>::value_type mysum(iter begin, iter end) {
    typename mytraits<iter>::value_type sum(0);
    for(iter i=begin; i!=end; ++i) sum += *i;
    return sum;
}

int main() {
    int v[4] = {1,2,3,4};
    std::cout << mysum(v, v+4) << '\n';
    std::cin.get(); return 0;
}
```
