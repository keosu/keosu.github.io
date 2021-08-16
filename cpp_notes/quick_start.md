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
