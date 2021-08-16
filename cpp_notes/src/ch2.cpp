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