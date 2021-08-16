# 0 C++简介
C++是一门面向对象的高级编程语言，由Bjane Stroustroup发明。

# 1 C++编译运行过程

一个简单的C++源文件：
```cpp
// main.cpp
#include<iostream>

int main(int argc,char** argv) {
  std::cout << "hello world" << std::endl;
  return 0;
}
// 编译运行命令（g++)
// g++ main.cpp && ./a.out
```
C++代码编译运行主要步骤：
- 对源文件进行预处理
- 预处理后的文件生成汇编代码
- 汇编文件生成目标文件
- 链接目标文件成可执行代码

当我们使用`g++ main.cpp`命令编译时，实际上包含了以上四个步骤，实际上我们可以通过选项来控制每次只执行其中一个步骤  
```bash
g++ -E main.cpp -o main.i #预处理，将头文件、宏展开
g++ -S main.i -o main.S #生成汇编代码
g++ -c main.S -o main.o #生成目标文件
g++ main.o -o main.out #链接成可执行文件（此处只有一个目标文件）
```

# 2 C++基本语法
以一个简单程序为例
```cpp
// ch2.cpp
#include<iostream>

// 类： 继承、封装、多态
class Base {
public:
  int ival;
  float fval;
  virtual void foo() {
    std::cout << "int Base foo()" << std::endl;
  }

};
class A: public Base {
public:
  void foo() {
    std::cout << "int A foo()" << std::endl;
  }
  void foo(int x) {
    std::cout << "int A foo(x)" << std::endl;
  } 
};
class B: public Base {
public: 
};
// 类测试
void class_test() {
  A a;
  B b;
  // a.foo();
  a.foo(2);

  b.foo();

  Base* base = new A;
  base->foo();
  delete(base);
}
// 函数定义
void test(bool isFoo) {
  if(isFoo) //条件语句
    std::cout << "Foo" << std::endl;
  else
    std::cout << "Bar" << std::endl;
}
void loop_test() {
  // 循环语句
  for(int i=0;i < 5;i++)
    std::cout << "number: " << i<< std::endl;

  int val = 2, endval = 10;
  while(val < endval) {
    std::cout << "val: " << val << std::endl;
    val += 2;
  }
}
int main(int argc,char** argv) {
  test(true);
  test(false);

  loop_test();

  class_test();

  return 0;
}

```
这个程序演示了C++的基本语法：
- 变量的类型和定义
- 基本控制语句：条件语句和循环语句
- 函数的定义和调用
- 类的基本使用：封装、继承和多态

# 3 关键字

## static
- 修饰变量  
函数内：表示静态变量，下次进入该函数维持上一次的值  
函数外：可以被模块内（一般指单个源文件）的函数访问，不能被其他模块（源文件）访问  
类的成员变量：表示静态成员，为类的所有实例共享

- 修饰函数：
模块内函数：可以被模块内的其他函数访问，不能被其他模块（源文件）访问  
类的成员函数：表静态函数，由`类名::函数名`使用，不能调用非静态函数

## const
- 表示只读，如果是想表示编译期常量，C++11后使用`constexpr`
- 函数参数能使用const尽量用const，确保不会被修改
- 类的成员函数用const修饰，表示函数内不会有修改操作

# 类

## 类的特殊成员函数
```cpp
class X {
  X();
  ~X();
  X(const X& x); // 拷贝构造
  X& operator=(const X& x); // 拷贝赋值
  X(X&& x); // 移动构造，C++11引入
  X& operator=(X&& x); // 移动赋值，C++11引入
};
```
**注意：**  
1. 当类的定义中没有显示定义时，编译器会默认生成 
2. 当你的设计明确不需要某个函数，使用delete禁用  
> ``` X(const X& x) = delete; ```  

3. 当你重载了一个构造函数，默认构造函数会被覆盖，如果需要，使用default  
> ``` X() = default; ```  

何时需要定义这些特殊的成员函数？有三个原则：
- 0原则：如果不需要自定义析构函数，通过除构造函数外其他五个都不需要定义
- 3原则：当需要自定义拷贝构造函数时，通常析构函数、拷贝赋值也需要定义 
- 5原则：当需要自定义移动构造函数时，通常析构函数、拷贝构造、拷贝赋值和移动赋值也需要定义 

## 多态
概念：
- 虚函数：使用virtual生命的类成员函数
- 纯虚函数：虚函数原型后加" = 0 "
- 抽象类：含有纯虚函数的类，不能直接实例化，一般用作接口定义
```cpp
class X {
  virtual void abc();
  virtual void def() = 0;
};
``` 
重载、隐藏和重写
- 重载overload: 同一类中，函数名相同，参数不同
- 隐藏override: 父子类中，函数名相同
- 覆盖/重写overwrite: 父子类中，相同签名的虚函数  
**注意**  
隐藏的父类函数，仍可以通过父类访问

多态：  
- 虚函数实现通过基类指针访问子类成员的运行时多态；  
- 重载可以看作编译时的多态  
- 另外，可以通过奇异递归模板模式（curiously recurring template pattern，CRTP）实现编译时多态：这种方式把派生类作为基类的模板参数
```cpp
template <typename T>
class Base {
public:
  void doWhat() {
    T& derived = static_cast<T&>(*this);
    // use derived...
  }
}; 
class Derived : public Base<Derived> {
  // ...
};
```
