# C++简介

C++ 是一种静态类型的、编译式的、通用的、大小写敏感的、不规则的编程语言，支持过程化编程、面向对象编程和泛型编程。  
C++ 是由Bjarne Stroustrup于1979年在贝尔实验室开始设计开发的。
主要的C++编译器有gcc、MSVC、clang等。


# C++编译运行过程

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

# C++基本语法
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

# 基本概念和关键字

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

## mutable
- 修饰类的成员变量：即使类的成员函数加了const修饰，也能修改该成员变量
- 修饰lambda函数：表示按值捕获的值是可修改的。

## volatile
volatile关键字用于告诉编译器该变量值是不稳定的，可能被更改。使用volatile注意事项：

- 编译器会对带有volatile关键字的变量禁用优化
- 当多个线程都要用到某一个变量且该变量的值会被改变时应该用volatile声明，该关键字的作用是防止编译器优化把变量从内存装入CPU寄存器中。如果变量被装入寄存器，那么多个线程有可能有的使用内存中的变量，有的使用寄存器中的变量，这会造成程序的错误执行。volatile的意思是让编译器每次操作该变量时一定要从内存中取出，而不是使用已经存在寄存器中的值(It cannot cache the variables in register)。
- 中断服务程序中访问到的变量最好带上volatile。
- 并行设备的硬件寄存器的变量最好带上volatile。
- 声明的变量可以同时带有const和volatile关键字。
- 多个volatile变量间的操作，是不会被编译器交换顺序的，能够保证volatile变量间的顺序性，编译器不会进行乱序优化(The value cannot change in order of assignment)。但volatile变量和非volatile变量之间的顺序，编译器不保证顺序，可能会进行乱序优化。

## 指针和引用
1. 指针是一个新的变量，存储了另一个变量的地址，我们可以通过访问这个地址来修改另一个变量；
   引用只是一个别名，还是变量本身，对引用的任何操作就是对变量本身进行操作，以达到修改变量的目的
2. 引用只有一级，而指针可以有多级
3. 指针传参的时候，还是值传递，指针本身的值不可以修改，需要通过解引用才能对指向的对象进行操作
引用传参的时候，传进来的就是变量本身，因此变量可以被修改

## 数组
```cpp
double a[10];
double b[5] = {1000.0, 2.0, 3.4, 17.0, 50.0};
// C++11 后引入std::array
std::array<double, 5> x = {1,2,3,4,5};
```

# 面向对象

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
- 隐藏override: 父子类中，函数名相同，参数相同，基类为虚函数
- 覆盖/重写overwrite: 父子类中，同名不同参，或者同名同参但基类非虚  
**注意**   隐藏的父类函数，仍可以通过父类访问

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

# 文件操作
```cpp
#include <fstream>
#include <iostream>
using namespace std;
int main() {
  char data[100];
  // open a file in write mode.
  ofstream outfile;
  outfile.open("afile.dat");
  cout << "Writing to the file" << endl;
  cout << "Enter your name: ";
  cin.getline(data, 100);
  // write inputted data into the file.
  outfile << data << endl;
  cout << "Enter your age: ";
  cin >> data;
  cin.ignore();
  // again write inputted data into the file.
  outfile << data << endl;
  // close the opened file.
  outfile.close();
  // open a file in read mode.
  ifstream infile;
  infile.open("afile.dat");
  cout << "Reading from the file" << endl;
  infile >> data;
  // write the data at the screen.
  cout << data << endl;
  // again read the data from the file and display it.
  infile >> data;
  cout << data << endl;
  // close the opened file.
  infile.close();
  return 0;
}
```
# 异常处理
```cpp
#include <iostream>
using namespace std;
double division(int a, int b) {
  if (b == 0) {
    throw "Division by zero condition!";
  }
  return (a / b);
}
int main() {
  int x = 50;
  int y = 0;
  double z = 0;
  try {
    z = division(x, y);
    cout << z << endl;
  } catch (const char* msg) {
    cerr << msg << endl;
  }
  return 0;
}
```

# 模板
模板是泛型编程的基础，即独立于特定类型的编程方式。  
模板主要分为函数模板，类模板。按照模板参数类别可分为类型模板和非类型模板。

函数模板实例：  
```cpp
include <iostream>

// 函数模板
template<typename T>
bool equivalent(const T& a, const T& b){
    return !(a < b) && !(b < a);
}
// 类模板
template<typename T=int> // 默认参数
class bignumber{
    T _v;
public:
    bignumber(T a) : _v(a) { }
    inline bool operator<(const bignumber& b) const; // 等价于 (const bignumber<T>& b)
};
// 在类模板外实现成员函数
template<typename T>
bool bignumber<T>::operator<(const bignumber& b) const{
    return _v < b._v;
}
// 非类型模板：MAX_SIZE
template<typename T, int MAX_SIZE>
struct Stack {
    void push(const T&);
    T pop();
    
private:
    T elems[MAX_SIZE];
    int size;
};
```
# 信号处理
信号是操作系统与进程通信的一种方式。  
```cpp
#include <csignal>
#include <iostream>
using namespace std;
void signalHandler(int signum) {
  cout << "Interrupt signal (" << signum << ") received.\n";
  // cleanup and close up stuff here
  // terminate program
  exit(signum);
}
int main() {
  int i = 0;
  // register signal SIGINT and signal handler
  signal(SIGINT, signalHandler);
  while (++i) {
    cout << "Going to sleep...." << endl;
    if (i == 3) {
      raise(SIGINT);
    } 
  }
  return 0;
}
```

# 并发编程
C++11引入了线程库thread用于创建一个执行的线程实例，所以它是一切并发编程的基础
```cpp
#include <iostream>
#include <thread>
int v = 1;
void critical_section(int change_v) {
  static std::mutex mtx;
  std::lock_guard<std::mutex> lock(mtx);
  // 执行竞争操作
  v = change_v;
  // 离开此作用域后 mtx 会被释放
}
int main() {
  std::thread t1(critical_section, 2), t2(critical_section, 3);
  t1.join();
  t2.join();
  std::cout << v << std::endl;
  return 0;
}
```