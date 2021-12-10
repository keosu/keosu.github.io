# TVM Schedule

TVM借鉴了Halide中计算逻辑和调度分离的思想。Schedule可看作一系列优化措施的集合，它不会改变计算结果，但通过选取合适的策略来提高计算的性能。  

TVM中关于调度原语（Schedule Primitives）的文档见[这里](https://tvm.apache.org/docs/how_to/work_with_schedules/schedule_primitives.html)。我们可以参考Halide中关于[Schedule](https://halide-lang.org/tutorials/tutorial_lesson_05_scheduling_1.html)的描述，它提供了可视化的动图，便于更直观的了解。

## 默认顺序

使用python定义一个简单的4x4矩阵复制操作，默认是行优先：
```python
A = te.placeholder((4, 4), name="A")
B = te.compute((4, 4), lambda i, j: A[i, j], name="B")
te.create_schedule([B.op]);
```
使用```tvm.lower```生成的中间代码
```c++
for (i: int32, 0, 4) {
    for (j: int32, 0, 4) {
      B_2[((i*4) + j)] = (float32*)A_2[((i*4) + j)]
    }
  }
```
![](res/lesson_05_row_major.gif)
 
## 改变顺序 reorder
改变坐标轴计算顺序：
```python
A = te.placeholder((4, 4), name="A")
B = te.compute((4, 4, lambda i, j: A[i, j], name="B"))
te.create_schedule([B.op]);
s[B].reorder(B.op.axis[1],B.op.axis[0])
```
使用```tvm.lower```生成的中间代码
```c++
for (j: int32, 0, 4) {
  for (i: int32, 0, 4) {
    B_2[((i*4) + j)] = (float32*)A_2[((i*4) + j)]
  }
}
```
![](res/lesson_05_col_major.gif)
 
## 拆分 Split
  
```python
A = te.placeholder((4, 4), name="A")
B = te.compute((4, 4, lambda i, j: A[i, j], name="B"))
te.create_schedule([B.op]);
s = te.create_schedule(B.op)
xo, xi = s[B].split(B.op.axis[0], factor=2)
print(tvm.lower(s, [A, B], simple_mode=True))
```
使用```tvm.lower```生成的中间代码
```c++
for (i.outer: int32, 0, 2) {
    for (i.inner: int32, 0, 2) {
      for (j: int32, 0, 4) {
        B_2[(((i.outer*8) + (i.inner*4)) + j)] = (float32*)A_2[(((i.outer*8) + (i.inner*4)) + j)]
      }
    }
  }
```
 
## 融合 fuse
融合是指对多个坐标轴进行融合操作
```python
A = te.placeholder((4, 4), name="A")
B = te.compute((4, 4, lambda i, j: A[i, j], name="B")) 
s = te.create_schedule(B.op) 
  
fused = s[B].fuse(B.op.axis[0], B.op.axis[1])

print(tvm.lower(s, [A, B], simple_mode=True))
```
使用```tvm.lower```生成的中间代码
```c++
for (i.j.fused: int32, 0, 16) {
    B_2[i.j.fused] = (float32*)A_2[i.j.fused]
  }
```
但是要知道，上述拆分和融合操作只是对Halide所能进行的操作进行一下演示而是，这种操作方式并没有实际用处，也就是说实际中的计算顺序并没有改变。

## 平铺 tile

前面的拆分和融合操作只是为了演示一下效果，通过查看生成生成的中间代码，它改变了写法，但并没有对实际的顺序产生影响。  
平铺操作中，我们将x和y轴以4为因子间隔进行划分，并且重新对计算的路径进行重排序：
```python
A = te.placeholder((16, 16), name="A")
B = te.compute((16, 16), lambda i, j: A[i, j], name="B")

s = te.create_schedule(B.op) 

xo, xi = s[B].split(B.op.axis[0], factor=4)
yo, yi = s[B].split(B.op.axis[1], factor=4)

print(tvm.lower(s, [A, B], simple_mode=True))
```
使用```tvm.lower```生成的中间代码
```c++
  for (i.outer: int32, 0, 4) {
    for (i.inner: int32, 0, 4) {
      for (j.outer: int32, 0, 4) {
        for (j.inner: int32, 0, 4) {
          B_2[((((i.outer*64) + (i.inner*16)) + (j.outer*4)) + j.inner)] = (float32*)A_2[((((i.outer*64) + (i.inner*16)) + (j.outer*4)) + j.inner)]
        }
      }
    }
  }
```

![](https://halide-lang.org/tutorials/figures/lesson_05_tiled.gif)
 
## 向量化 vectorize
向量化利用CPU的SIMD特性实现加速
```python
A = te.placeholder((8, 4), name="A")
B = te.compute((8, 4), lambda i, j: A[i, j], name="B")

s = te.create_schedule(B.op) 
outer, inner = s[B].split(B.op.axis[0], factor=4)
s[B].vectorize(inner)

print(tvm.lower(s, [A, B], simple_mode=True))
```
使用```tvm.lower```生成的中间代码
```c++
  for (i.outer: int32, 0, 2) {
    for (j: int32, 0, 4) {
      B_2[ramp(((i.outer*16) + j), 4, 4)] = (float32x4*)A_2[ramp(((i.outer*16) + j), 4, 4)]
    }
  }
```

![](https://halide-lang.org/tutorials/figures/lesson_05_vectors.gif)

## 展开 unrolling
循环展开也是一种常见的优化手段。

```python
A = te.placeholder((4, 4), name="A")
B = te.compute((4, 4), lambda i, j: A[i, j], name="B")

s = te.create_schedule(B.op)  
s[B].unroll(B.op.axis[0])

print(tvm.lower(s, [A, B], simple_mode=True))
```
使用```tvm.lower```生成的中间代码
```c++ 
  for (j: int32, 0, 4) {
    B_2[j] = (float32*)A_2[j]
  }
  for (j_1: int32, 0, 4) {
    B_2[(j_1 + 4)] = (float32*)A_2[(j_1 + 4)]
  }
  for (j_2: int32, 0, 4) {
    B_2[(j_2 + 8)] = (float32*)A_2[(j_2 + 8)]
  }
  for (j_3: int32, 0, 4) {
    B_2[(j_3 + 12)] = (float32*)A_2[(j_3 + 12)]
  }
```
## 绑定 bind

可以绑定特定轴到线程，通常用于GPU，如果需要充分的利用并行的性能，那么就需要将计算任务合理地分配到每个计算单元中。
```python
A = tvm.placeholder((n,), name='A')
B = tvm.compute(A.shape, lambda i: A[i] * 2, name='B')

s = tvm.create_schedule(B.op)
bx, tx = s[B].split(B.op.axis[0], factor=64)
s[B].bind(bx, tvm.thread_axis("blockIdx.x"))
s[B].bind(tx, tvm.thread_axis("threadIdx.x"))
print(tvm.lower(s, [A, B], simple_mode=True))
```
以下面的代码为例，基于3.1中的例子，此时得到的ir为两层for循环。假设n的值为128，那么两层for循环分别为[0,2)与[0,64)。将[0,2)这根轴绑定blockIdx.x，那么就可以理解为，将该计算任务分配到2个核心中进行计算，每个核心的计算量是[0,64)这个for循环的计算量。

基于核心的粒度分配好任务量后，在每个核心中，处理的是[0,64)这个for循环，将该轴绑定到threadIdx.x，那么就可以理解为每个核开启多线程，每个线程处理的内容为[0,64)这个for循环中的任务（本例子中任务就是一个乘以2的操作）。

```c++
produce B {
  // attr [iter_var(blockIdx.x, , blockIdx.x)] thread_extent = ((n + 63)/64)
  // attr [iter_var(threadIdx.x, , threadIdx.x)] thread_extent = 64
  if (likely((((blockIdx.x*64) + threadIdx.x) < n))) {
    if (likely((((blockIdx.x*64) + threadIdx.x) < n))) {
      B[((blockIdx.x*64) + threadIdx.x)] = (A[((blockIdx.x*64) + threadIdx.x)]*2.000000f)
    }
  }
}
```

## compute_at

对于由多个操作组合成的Schedule，compate_at可以改变计算的位置。  

如下的例子中，计算的内容为B = A + 1与C = B * 2，且为element-wise的计算。默认的schedule会使用两个for循环，在第1个for循环中计算B = A + 1，完成后才用新的for循环计算C = B * 2。
明明一个for循环能完成的事，为啥要分成多个（注意：是多个而不是多层）for循环串行执行？使用compute_at合并。
```python
A = tvm.placeholder((m,), name='A')
B = tvm.compute((m,), lambda i: A[i]+1, name='B')
C = tvm.compute((m,), lambda i: B[i]*2, name='C')

s = tvm.create_schedule(C.op)
s[B].compute_at(s[C], C.op.axis[0]) #合并
print(tvm.lower(s, [A, B, C], simple_mode=True))
```
生成的中间代码：  
```c++
// 合并前
  for (i, 0, m) {
    B[i] = (A[i] + 1.000000f)
  }  
  for (i, 0, m) {
    C[i] = (B[i]*2.000000f)
  } 
// 合并后
  for (i, 0, m) { 
    B[i] = (A[i] + 1.000000f) 
    C[i] = (B[i]*2.000000f)
  }
}
```
## compute_inline

有点类似于c++中inline的作用，省略额外的中间开销
```python
A = tvm.placeholder((m,), name='A')
B = tvm.compute((m,), lambda i: A[i]+1, name='B')
C = tvm.compute((m,), lambda i: B[i]*2, name='C')

s = tvm.create_schedule(C.op)
s[B].compute_inline()
print(tvm.lower(s, [A, B, C], simple_mode=True))
```
生成的中间结果，只有一个步骤： 
```c++
  for (i, 0, m) {
    C[i] = ((A[i] + 1.000000f)*2.000000f)
  }
}
```

## compute_root

让schedule恢复到默认的状态。

```python
A = tvm.placeholder((m,), name='A')
B = tvm.compute((m,), lambda i: A[i]+1, name='B')
C = tvm.compute((m,), lambda i: B[i]*2, name='C')

s = tvm.create_schedule(C.op)
s[B].compute_at(s[C], C.op.axis[0])
s[B].compute_root()
print(tvm.lower(s, [A, B, C], simple_mode=True))
```
```c++ 
  for (i, 0, m) {
    B[i] = (A[i] + 1.000000f)
  } 
  for (i, 0, m) {
    C[i] = (B[i]*2.000000f)
  } 
``` 


## 总结

上面简要介绍了TVM中基本的调度原语（Schedule Primitives），我们可以通过组合这些基本元素，达到一个满意的优化结果。

 