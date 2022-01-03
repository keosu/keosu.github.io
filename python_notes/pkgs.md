# 主要包

# Numpy

NumPy是一个功能强大的Python库，主要用于对多维数组执行计算。  
Numpy中最重要的一个数据类型就是numpy.ndarray,可以很方便地对其进行各种数学运算。  

## 数组创建

```python
import numpy as np

a = np.array([1, 2, 3])   # Create a rank 1 array
print(type(a))            # Prints "<class 'numpy.ndarray'>"
print(a.dtype)            # Prints "dtype('int64')"
print(a.shape)            # Prints "(3,)"
print(a[0], a[1], a[2])   # Prints "1 2 3"
a[0] = 5                  # Change an element of the array

# Numpy还提供了许多创建数组的函数： 
a = np.zeros((2,2))   # Create an array of all zeros 
b = np.ones((1,2))    # Create an array of all ones 
c = np.full((2,2), 7)  # Create a constant array 
d = np.eye(2)         # Create a 2x2 identity matrix 
e = np.random.random((2,2))  # Create an array filled with random values 
f = np.arange(0, 100, 10)    #  [0 10 20 30 40 50 60 70 80 90]
```

## 数组切片
```python
a = np.array([[11, 12, 13, 14, 15],
              [16, 17, 18, 19, 20],
              [21, 22, 23, 24, 25],
              [26, 27, 28 ,29, 30],
              [31, 32, 33, 34, 35]])

print(a[0, 1:4]) # >>>[12 13 14]
print(a[1:4, 0]) # >>>[16 21 26]
print(a[::2,::2]) # >>>[[11 13 15]
                  #     [21 23 25]
                  #     [31 33 35]]
print(a[:, 1]) # >>>[12 17 22 27 32]

## 使用列表进行索引
a = np.arange(0, 100, 10)
indices = [1, 5, -1]
b = a[indices]
print(a) # >>>[ 0 10 20 30 40 50 60 70 80 90]
print(b) # >>>[10 50 90]

## 布尔屏蔽
x = np.random.random((5,5))
cond = x>=0.5
x[cond]
# 或者
x[x>=0/5]
# 获取满足条件的索引
np.where(x>0.5)
```

## 数组属性 
```python
## Numpy数组基本的属性
print(type(a)) # >>><class 'numpy.ndarray'>
print(a.dtype) # >>>int64
print(a.size) # >>>25
print(a.shape) # >>>(5, 5)
print(a.itemsize) # >>>8
print(a.ndim) # >>>2
print(a.nbytes) # >>>200
```

## 数组变换
```python
array.reshape((2,3)) #
array.flatten()      # 平铺为一维数组，拷贝
array.ravel()        # 平铺为一维数组,不拷贝
array.squeeze()      # 删除某个维度，该维度长度必须为1
array.transpose()    # 重排数据
array.padding() 
```
