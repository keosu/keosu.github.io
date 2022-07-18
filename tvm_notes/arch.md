
# 基础：Python和C++相互调用

在TVM中，Python和C++相互调用的基础是Python内置的ctype，它只支持c-style函数。  
TVM中通过封装成PackedFunc类实现了更灵活的跨语言交互（另外还有一个TypedPackedFunc类，增加了类型检查的功能）
    
## PackedFunc相关的类   
- TVMValue : union，储存c++和其它语言交互时所支持的几种类型的数据 
- TVMArgs  : 封装传给PackedFunc的所有参数，主要成员TVMValue、参数类型编码、参数个数
- TVMPODValue_ ：PackedFunc值类型基类，主要处理POD类型的数据 
- TVMArgValue  ：继承自TVMPODValue_类，用作表示PackedFunc的一个参数，与TVMPODValue_的区别是扩充了一些数据类型的支持
- TVMRetValue  ：继承自TVMPODValue_类，主要作用是作为存放调用PackedFunc返回值的容器
- TVMArgsSetter：给TVMValue对象赋值的辅助类   

PackedFunc的实现很简单，内部只使用了一个储存函数指针的变量，再通过重载函数调用运算符，调用这个函数指针所指向的函数： 
```c++ 
class PackedFunc {
public:  
  using FType = function<void(TVMArgs args, TVMRetValue* rv)>;  
  PackedFunc() {}  
  explicit PackedFunc(FType body) : body_(body) {}

  template <typename… Args>  
  inline TVMRetValue operator()(Args&&… args) const {
    const int kNumArgs = sizeof…(Args);  
    const int kArraySize = kNumArgs > 0 ? kNumArgs : 1;  
    TVMValue values[kArraySize];  
    int type_codes[kArraySize];  
    detail::for_each(TVMArgsSetter(values, type_codes),  
    std::forward(args)…);  
    TVMRetValue rv;  
    body_(TVMArgs(values, type_codes, kNumArgs), &rv);  
    return rv;  
  } 
  inline void CallPacked(TVMArgs args, TVMRetValue* rv) const {
    body_(args, rv);  
  } 
private:  
  FType body_;  
};  
```

## 注册函数

最主要的一个函数注册宏是TVM_REGISTER_GLOBAL 
 
注册的函数可以是普通函数，也可以是labda表达式，注册接口有三个：set_body、set_body_typed、set_body_method，
第一个使用的是PackedFunc，后面两个使用的是TypedPackedFunc。

```c++
    //使用set_body接口注册lambda表达式：  
    // src/topi/nn.cc  
    TVM_REGISTER_GLOBAL(“topi.nn.relu”).set_body([](TVMArgs args, TVMRetValue* rv) {  
      *rv = relu(args[0]);  
    });  
    //使用set_body_typed接口注册lambda表达式：  
    // src/te/schedule/graph.cc  
    TVM_REGISTER_GLOBAL(“schedule.PostDFSOrder”).set_body_typed([](  
      const Array& roots,  
      const ReadGraph& g) {  
      return PostDFSOrder(roots, g);  
    });  
    //使用set_body_method接口注册类内函数：  
    // src/ir/module.cc  
    TVM_REGISTER_GLOBAL(“ir.Module_GetGlobalVar”).set_body_method(&IRModuleNode::GetGlobalVar);  
    //
    ```
TVM_REGISTER_GLOBAL宏在注册文件定义了一个static的引用变量，引用到注册机内部new出来的一个新的Registry对象： 
```c++ 
// include/tvm/runtime/registry.h  
#define TVM_REGISTER_GLOBAL(OpName)  
  static ::tvm::runtime::Registry& __mk_TVM ## __COUNTER__ =  ::tvm::runtime::Registry::Register(OpName)  
 
// src/runtime/registry.cc  
struct Registry::Manager {  
      static Manager* Global() {  
      static Manager* inst = new Manager();  
      return inst;  
    }  
    std::mutex mutex;  
    unordered_map<std::string, Registry*> fmap;  
};  

class Registry {  
public:  
    Registry& set_body(PackedFunc f);  
    Registry& set_body_typed(FLambda f);  
    Registry& set_body_method(R (T::*f)(Args…));

    static Registry& Register(const std::string& name);  
    static const PackedFunc* Get(const std::string& name);  
    static std::vector ListNames();

    protected:  
    std::string name_;  
    PackedFunc func_;  
    friend struct Manager;  
};   

// 注册函数
Registry& Registry::Register(const std::string& name) {
  Manager* m = Manager::Global();  
  std::lock_guardstd::mutex lock(m->mutex);

  Registry* r = new Registry();  
  r->name_ = name;  
  m->fmap[name] = r;  
  return _r;  
}  
 // 获取注册函数： 
const PackedFunc_ Registry::Get(const std::string& name) {
  Manager* m = Manager::Global();  
  std::lock_guardstd::mutex lock(m->mutex);  
  auto it = m->fmap.find(name);  
  if (it == m->fmap.end()) return nullptr;  
  return &(it->second->func_);  
} 
```

# 基础类:Object , 容器

一、Object、ObjectPtr、ObjectRef
------------------------------
 
*   Object相当于控制块，可以通过引用计数ref_counter_来控制对象的生命周期，对象的[析构函数]也可以通过deleter_这个函数指针指定Object的子类的除去Object基类的部分相当于数据块，里面保存有类的真实数据
*   ObjectRef就像是shared_ptr这个wrapper，自身不包含实际数据，但是可以操作实际的数据
*   ObjectPtr的作用在使用的角度有点类似ObjectRef，不同的是数据类型，ObjectPtr<T>是一个模板，下面还会详细讲ObjectPtr
 
```cpp
template <typename T, typename... Args>
inline ObjectPtr<T> make_object(Args&&... args) {
  return SimpleObjAllocator().make_object<T>(std::forward<Args>(args)...);
}
```
 `
 
```cpp

template <typename T> 
class ObjectPtr {
public:
  template <typename Y>
  ObjectPtr(ObjectPtr<Y>&& other) : data_(other.data_) {
    static_assert(std::is_base_of<T, Y>::value, "error");
    other.data_ = nullptr;
  }
private:
  Object* data_{nullptr};
};


class ObjectRef {
public:
  ObjectRef(Object *data) {
    data_ = data;
    data_->IncRef(); 
  }
  ObjectRef(const ObjectRef& other) {
    data_ = other.get();
    data_->IncRef(); 
  }
  ~ObjectRef() { data_->IncRef(); }
  ObjectRef& operator=(const ObjectRef& other) {
    data_->DecRef();
    data_ = other.get();
    data_->IncRef();
  }
  
  const Object* get() const { return data_.get(); }
  const Object* operator->() const { return get(); }
  
private:
  Object *data_{nullptr};
};
```
 

四、Summary and Reference
-----------------------

我觉得Object机制最主要的优点有两个：

1.  统一抽象：这个很好理解，所有的类都继承自Object，简化了很多接口设计，整个系统的可扩展性很好
2.  性能优势：这个重点说下，我感觉这个优势主要来自于Object机制中的下面两个实现

*   简单的智能指针：Object机制中对于对象生命周期的管理就像是一个共享所有权类型的智能指针，虽然没有shared_ptr强大，但是通过对源码的分析，应该比shared_ptr快，之前《[内存管理：几个简单测试（二）](https://link.zhihu.com/?target=http%3A//mp.weixin.qq.com/s%3F__biz%3DMzg5MzU4NTU5Nw%3D%3D%26mid%3D2247484020%26idx%3D1%26sn%3D2382cd03029e87240ec779c9580c7faa%26chksm%3Dc02dd3a6f75a5ab0452b269bc251daa62b3de28f78d4829f5e22e177dc14c7c198469d9d69e1%26scene%3D21%23wechat_redirect)》中也有对shared_ptr的测试，shared_ptr的开销确实挺大的。
*   简单的RTTI：Object机制中使用一个整数值type_index_来标识当前的类类型，继承体系中的所有类的type_index_值可以简单认为在编译时就已经确定好，这显然比使用C++虚函数机制中的RTTI开销要小

##

- String
- Array
- Map
- ShapeTuple
- Optional


