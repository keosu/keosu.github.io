
```cpp
template<typename F, size_t... I, typename ... Args>
inline auto tuple_apply_impl(const F& f, const std::index_sequence<I...>&, const std::tuple<Args...>& tp)
{
    return f(std::get<I>(tp)...);
}

template<typename F, typename ... Args>
inline auto tuple_apply(const F& f, const std::tuple<Args...>& tp) -> decltype(f(std::declval<Args>()...))
{
    return tuple_apply_impl(f, std::make_index_sequence<sizeof... (Args)>{}, tp);
}

int main()
{
//test code
auto f = [](int x, int y, int z) { return x + y - z; };
//将函数调用需要的参数保存到tuple中
auto params = make_tuple(1, 2, 3);

//将保存的参数传给函数f，实现函数调用
auto result = tuple_apply(f, params); //result is 0

return 0;
}
```


```cpp
#include<functional>

#define define_functor_type(func_name) class tfn_##func_name {\
public: template <typename... Args> auto operator()(Args&&... args) const \
    ->decltype(func_name(std::forward<Args>(args)...))\
{ return func_name(std::forward<Args>(args)...); } }

using namespace std;
//test code
int add(int a, int b)
{
    return a + b;
}
int add_one(int a) 
{ 
return 1 + a; 
}

define_functor_type(add);
define_functor_type(add_one);

int main()
{
    tfn_add add_functor;
    add_functor(1, 2); //result is 3

    tfn_add_one add_one_functor;
    add_one_functor(1); //result is 2

    return 0;
}
```