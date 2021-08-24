# Effective Modern C++ Notes

# 类型推导
模板类型推导
有三种情况
- 引用或指针: T忽略引用或指针，形参类型从实参得出
- 通用引用：除了纯右值除掉引用，其它T带引用
- 不是引用或指针：按值传递，cv修饰符被去掉
- 数组和函数退化为指针

auto
- auto类型推导通常和模板类型推导相同，除了假定花括号初始化代表std::initializer_list
- 在C++14中auto允许出现在函数返回值或者lambda函数形参中，它的工作机制是模板类型推导

decltype
- 对于T类型非单纯变量名的左值表达式，decltype产生的是 T&
- 使用deltype(auto)需注意

可以借助工具查看推导类型，主要方式如下，但重要的还是理解推导规则  
- IDE
- 编译器报错信息
- 第三方库，如boost TypeIndex