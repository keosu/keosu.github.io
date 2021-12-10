# TE 
## TE基础实例: Add  

TVM使用Tensor Expression(TE)来定义计算、进行优化。  
TE使用函数式编程来描述计算，没有副作用。每一个算子都能够用一个TE表示。  

```python 
import tvm
import tvm.testing
from tvm import te
import numpy as np

# target越具体越能发挥特定硬件优势
# 如可以使用"llvm -mcpu=skylake-avx512"如果支持AVX-512指令
tgt = tvm.target.Target(target="llvm", host="llvm")

#######
# 定义计算（实际的计算不会发生在这个阶段）  
n = te.var("n")
A = te.placeholder((n,), name="A")
B = te.placeholder((n,), name="B")
C = te.compute(A.shape, lambda i: A[i] + B[i], name="C") 
#######
# schedule 描述怎样进行计算,这里使用默认方式 
s = te.create_schedule(C.op)

######################################################################
# 编译运行
fadd = tvm.build(s, [A, B, C], tgt, name="myadd")
# 运行并和numpy结果比较
dev = tvm.device(tgt.kind.name, 0)
n = 1024
a = tvm.nd.array(np.random.uniform(size=n).astype(A.dtype), dev)
b = tvm.nd.array(np.random.uniform(size=n).astype(B.dtype), dev)
c = tvm.nd.array(np.zeros(n, dtype=C.dtype), dev)
fadd(a, b, c)
tvm.testing.assert_allclose(c.numpy(), a.numpy() + b.numpy())
```
## 手动优化Add
```python

# 定义评估函数
import timeit
np_repeat = 100
np_running_time = timeit.timeit(
    setup="import numpy\n"
    "n = 32768\n"
    'dtype = "float32"\n'
    "a = numpy.random.rand(n, 1).astype(dtype)\n"
    "b = numpy.random.rand(n, 1).astype(dtype)\n",
    stmt="answer = a + b",
    number=np_repeat,
)
print("Numpy running time: %f" % (np_running_time / np_repeat))


def evaluate_addition(func, target, optimization, log):
    dev = tvm.device(target.kind.name, 0)
    n = 32768
    a = tvm.nd.array(np.random.uniform(size=n).astype(A.dtype), dev)
    b = tvm.nd.array(np.random.uniform(size=n).astype(B.dtype), dev)
    c = tvm.nd.array(np.zeros(n, dtype=C.dtype), dev)

    evaluator = func.time_evaluator(func.entry_name, dev, number=10)
    mean_time = evaluator(a, b, c).mean
    print("%s: %f" % (optimization, mean_time))

    log.append((optimization, mean_time))


log = [("numpy", np_running_time / np_repeat)]
evaluate_addition(fadd, tgt, "naive", log=log)

################################################################################
# 使用Paralleism优化Schedule
s[C].parallel(C.op.axis[0]) 
# tvm.lower 查看 IR， simple_mode=True返回可读表示 
print(tvm.lower(s, [A, B, C], simple_mode=True))
 
fadd_parallel = tvm.build(s, [A, B, C], tgt, name="myadd_parallel")
fadd_parallel(a, b, c)

tvm.testing.assert_allclose(c.numpy(), a.numpy() + b.numpy())
evaluate_addition(fadd_parallel, tgt, "parallel", log=log)

################################################################################
# 使用Vectorization （SIMD）来优化Schedule 

n = te.var("n")
A = te.placeholder((n,), name="A")
B = te.placeholder((n,), name="B")
C = te.compute(A.shape, lambda i: A[i] + B[i], name="C")

s = te.create_schedule(C.op)
 
factor = 4
outer, inner = s[C].split(C.op.axis[0], factor=factor)
s[C].parallel(outer)
s[C].vectorize(inner)

fadd_vector = tvm.build(s, [A, B, C], tgt, name="myadd_parallel")

evaluate_addition(fadd_vector, tgt, "vector", log=log) 
print(tvm.lower(s, [A, B, C], simple_mode=True))

# 比较结果
baseline = log[0][1]
print("%s\t%s\t%s" % ("Operator".rjust(20), "Timing".rjust(20), "Performance".rjust(20)))
for result in log:
    print(
        "%s\t%s\t%s"
        % (result[0].rjust(20), str(result[1]).rjust(20), str(result[1] / baseline).rjust(20))
    )
```


## Add模块的保存和加载

通过TVM编译后的模块，可以导出为库供以后使用。  
使用环境只需轻量运行时环境，无需编译器部分。

主要代码：  
```python
from tvm.contrib import cc 
from tvm.contrib import utils

# 法一：目标分别保存，再创建动态库
temp = utils.tempdir()
fadd.save(temp.relpath("myadd.o"))
fadd.imported_modules[0].save(temp.relpath("myadd.ptx")) # 导入平台相关信息
cc.create_shared(temp.relpath("myadd.so"), [temp.relpath("myadd.o")])
# 法二：一次打包
fadd.export_library(temp.relpath("myadd_pack.so"))

# 加载
fadd_load = tvm.runtime.load_module(temp.relpath("myadd.so"))
```

完整示例：  
```python 
from tvm.contrib import cc
from tvm.contrib import utils

temp = utils.tempdir()
fadd.save(temp.relpath("myadd.o"))
# 针对不同平台，需要保存额外的设备相关信息
if tgt.kind.name == "cuda":
    fadd.imported_modules[0].save(temp.relpath("myadd.ptx"))
if tgt.kind.name == "rocm":
    fadd.imported_modules[0].save(temp.relpath("myadd.hsaco"))
if tgt.kind.name.startswith("opencl"):
    fadd.imported_modules[0].save(temp.relpath("myadd.cl"))
cc.create_shared(temp.relpath("myadd.so"), [temp.relpath("myadd.o")])
print(temp.listdir())

# 加载模型
fadd1 = tvm.runtime.load_module(temp.relpath("myadd.so"))
if tgt.kind.name == "cuda":
    fadd1_dev = tvm.runtime.load_module(temp.relpath("myadd.ptx"))
    fadd1.import_module(fadd1_dev)

if tgt.kind.name == "rocm":
    fadd1_dev = tvm.runtime.load_module(temp.relpath("myadd.hsaco"))
    fadd1.import_module(fadd1_dev)

if tgt.kind.name.startswith("opencl"):
    fadd1_dev = tvm.runtime.load_module(temp.relpath("myadd.cl"))
    fadd1.import_module(fadd1_dev)

fadd1(a, b, c)
tvm.testing.assert_allclose(c.numpy(), a.numpy() + b.numpy())

# 或者一次打包
fadd.export_library(temp.relpath("myadd_pack.so"))
fadd2 = tvm.runtime.load_module(temp.relpath("myadd_pack.so"))
fadd2(a, b, c)
tvm.testing.assert_allclose(c.numpy(), a.numpy() + b.numpy())


################################################################################
# Generate OpenCL Code

if tgt.kind.name.startswith("opencl"):
    fadd_cl = tvm.build(s, [A, B, C], tgt, name="myadd")
    print("------opencl code------")
    print(fadd_cl.imported_modules[0].get_source())
    dev = tvm.cl(0)
    n = 1024
    a = tvm.nd.array(np.random.uniform(size=n).astype(A.dtype), dev)
    b = tvm.nd.array(np.random.uniform(size=n).astype(B.dtype), dev)
    c = tvm.nd.array(np.zeros(n, dtype=C.dtype), dev)
    fadd_cl(a, b, c)
    tvm.testing.assert_allclose(c.numpy(), a.numpy() + b.numpy()) 
```




## 示例：矩阵乘的实现和优化
```python
################################################################################ 
# 矩阵乘MatMul是一个计算密集性任务，在CPU上通常有两类主要的优化策略：
# 1. 提高缓存命中率. 
# 2. 使用向量处理单元(利用SIMD)  

import tvm
import tvm.testing
from tvm import te
import numpy

# The size of the matrix  (M, K) x (K, N) 
M = 1024
K = 1024
N = 1024 
dtype = "float32"   
target = tvm.target.Target(target="llvm", host="llvm")
dev = tvm.device(target.kind.name, 0)

# 随机输入数据
a = tvm.nd.array(numpy.random.rand(M, K).astype(dtype), dev)
b = tvm.nd.array(numpy.random.rand(K, N).astype(dtype), dev)

# 使用numpy获得一个性能基准
np_repeat = 100
np_running_time = timeit.timeit(
    setup="import numpy\n"
    "M = " + str(M) + "\n"
    "K = " + str(K) + "\n"
    "N = " + str(N) + "\n"
    'dtype = "float32"\n'
    "a = numpy.random.rand(M, K).astype(dtype)\n"
    "b = numpy.random.rand(K, N).astype(dtype)\n",
    stmt="answer = numpy.dot(a, b)",
    number=np_repeat,
)
print("Numpy running time: %f" % (np_running_time / np_repeat))

answer = numpy.dot(a.numpy(), b.numpy())
 
# 使用 TE来定义矩阵乘运算
k = te.reduce_axis((0, K), "k")
A = te.placeholder((M, K), name="A")
B = te.placeholder((K, N), name="B")
C = te.compute((M, N), lambda x, y: te.sum(A[x, k] * B[k, y], axis=k), name="C")

# 使用默认schedule
s = te.create_schedule(C.op)
func = tvm.build(s, [A, B, C], target=target, name="mmult")

c = tvm.nd.array(numpy.zeros((M, N), dtype=dtype), dev)
func(a, b, c)
tvm.testing.assert_allclose(c.numpy(), answer, rtol=1e-5)


def evaluate_operation(s, vars, target, name, optimization, log):
    func = tvm.build(s, [A, B, C], target=target, name="mmult")
    assert func

    c = tvm.nd.array(numpy.zeros((M, N), dtype=dtype), dev)
    func(a, b, c)
    tvm.testing.assert_allclose(c.numpy(), answer, rtol=1e-5)

    evaluator = func.time_evaluator(func.entry_name, dev, number=10)
    mean_time = evaluator(a, b, c).mean
    print("%s: %f" % (optimization, mean_time))
    log.append((optimization, mean_time))

log = []
evaluate_operation(s, [A, B, C], target=target, name="mmult", optimization="none", log=log) 
print(tvm.lower(s, [A, B, C], simple_mode=True))

################################################################################
# 优化1: Blocking
# ~~~~~~~~~~~~~~~~~~~~~~~~
#
# A important trick to enhance the cache hit rate is blocking, where you
# structure memory access such that the inside a block is a small neighborhood
# that has high memory locality. In this tutorial, we pick a block factor of
# 32. This will result in a block that will fill a 32 * 32 * sizeof(float) area
# of memory. This corresponds to a cache size of 4KB, in relation to a
# reference cache size of 32 KB for L1 cache.
#
# We begin by creating a default schedule for the ``C`` operation, then apply a
# ``tile`` scheduling primitive to it with the specified block factor, with the
# scheduling primitive returning the resulting loop order from outermost to
# innermost, as a vector ``[x_outer, y_outer, x_inner, y_inner]``. We then get
# the reduction axis for output of the operation, and perform a split operation
# on it using a factor of 4. This factor doesn't directly impact the blocking
# optimization we're working on right now, but will be useful later when we
# apply vectorization.
#
# Now that the operation has been blocked, we can reorder the computation to
# put the reduction operation into the outermost loop of the computation,
# helping to guarantee that the blocked data remains in cache. This completes
# the schedule, and we can build and test the performance compared to the naive
# schedule.

bn = 32

# Blocking by loop tiling
xo, yo, xi, yi = s[C].tile(C.op.axis[0], C.op.axis[1], bn, bn)
(k,) = s[C].op.reduce_axis
ko, ki = s[C].split(k, factor=4)

# Hoist reduction domain outside the blocking loop
s[C].reorder(xo, yo, ko, ki, xi, yi)

evaluate_operation(s, [A, B, C], target=target, name="mmult", optimization="blocking", log=log)

################################################################################
# By reordering the computation to take advantage of caching, you should see a
# significant improvement in the performance of the computation. Now, print the
# internal representation and compare it to the original:

print(tvm.lower(s, [A, B, C], simple_mode=True))

################################################################################
# 优化 2: Vectorization
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# Another important optimization trick is vectorization. When the memory access
# pattern is uniform, the compiler can detect this pattern and pass the
# continuous memory to the SIMD vector processor. In TVM, we can use the
# ``vectorize`` interface to hint the compiler this pattern, taking advantage
# of this hardware feature.
#
# In this tutorial, we chose to vectorize the inner loop row data since it is
# already cache friendly from our previous optimizations.

# Apply the vectorization optimization
s[C].vectorize(yi)

evaluate_operation(s, [A, B, C], target=target, name="mmult", optimization="vectorization", log=log)

# The generalized IR after vectorization
print(tvm.lower(s, [A, B, C], simple_mode=True))

################################################################################
# 优化 3: Loop Permutation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# If we look at the above IR, we can see the inner loop row data is vectorized
# and B is transformed into PackedB (this is evident by the `(float32x32*)B2`
# portion of the inner loop). The traversal of PackedB is sequential now. So we
# will look at the access pattern of A. In current schedule, A is accessed
# column by column which is not cache friendly. If we change the nested loop
# order of `ki` and inner axes `xi`, the access pattern for A matrix will be
# more cache friendly.

s = te.create_schedule(C.op)
xo, yo, xi, yi = s[C].tile(C.op.axis[0], C.op.axis[1], bn, bn)
(k,) = s[C].op.reduce_axis
ko, ki = s[C].split(k, factor=4)

# re-ordering
s[C].reorder(xo, yo, ko, xi, ki, yi)
s[C].vectorize(yi)

evaluate_operation(
    s, [A, B, C], target=target, name="mmult", optimization="loop permutation", log=log
)

# Again, print the new generalized IR
print(tvm.lower(s, [A, B, C], simple_mode=True))

################################################################################
# 优化 4: Array Packing
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# Another important trick is array packing. This trick is to reorder the
# storage dimension of the array to convert the continuous access pattern on
# certain dimension to a sequential pattern after flattening.
#
# .. image:: https://github.com/dmlc/web-data/raw/main/tvm/tutorial/array-packing.png
#    :align: center
#
# Just as it is shown in the figure above, after blocking the computations, we
# can observe the array access pattern of B (after flattening), which is
# regular but discontinuous. We expect that after some transformation we can
# get a continuous access pattern. By reordering a ``[16][16]`` array to a
# ``[16/4][16][4]`` array the access pattern of B will be sequential when
# grabing the corresponding value from the packed array.
#
# To accomplish this, we are going to have to start with a new default
# schedule, taking into account the new packing of B. It's worth taking a
# moment to comment on this: TE is a powerful and expressive language for
# writing optimized operators, but it often requires some knowledge of the
# underlying algorithm, data structures, and hardware target that you are
# writing for. Later in the tutorial, we will discuss some of the options for
# letting TVM take that burden. Regardless, let's move on with the new
# optimized schedule.

# We have to re-write the algorithm slightly.
packedB = te.compute((N / bn, K, bn), lambda x, y, z: B[y, x * bn + z], name="packedB")
C = te.compute(
    (M, N),
    lambda x, y: te.sum(A[x, k] * packedB[y // bn, k, tvm.tir.indexmod(y, bn)], axis=k),
    name="C",
)

s = te.create_schedule(C.op)

xo, yo, xi, yi = s[C].tile(C.op.axis[0], C.op.axis[1], bn, bn)
(k,) = s[C].op.reduce_axis
ko, ki = s[C].split(k, factor=4)

s[C].reorder(xo, yo, ko, xi, ki, yi)
s[C].vectorize(yi)

x, y, z = s[packedB].op.axis
s[packedB].vectorize(z)
s[packedB].parallel(x)

evaluate_operation(s, [A, B, C], target=target, name="mmult", optimization="array packing", log=log)

# Here is the generated IR after array packing.
print(tvm.lower(s, [A, B, C], simple_mode=True))

################################################################################
# 优化 5: Optimizing Block Writing Through Caching
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# Up to this point all of our optimizations have focused on efficiently
# accessing and computing the data from the `A` and `B` matrices to compute the
# `C` matrix. After the blocking optimization, the operator will write result
# to `C` block by block, and the access pattern is not sequential. We can
# address this by using a sequential cache array, using a combination of
# `cache_write`, `compute_at`, and `unroll`to hold the block results and write
# to `C` when all the block results are ready.

s = te.create_schedule(C.op)

# Allocate write cache
CC = s.cache_write(C, "global")

xo, yo, xi, yi = s[C].tile(C.op.axis[0], C.op.axis[1], bn, bn)

# Write cache is computed at yo
s[CC].compute_at(s[C], yo)

# New inner axes
xc, yc = s[CC].op.axis

(k,) = s[CC].op.reduce_axis
ko, ki = s[CC].split(k, factor=4)
s[CC].reorder(ko, xc, ki, yc)
s[CC].unroll(ki)
s[CC].vectorize(yc)

x, y, z = s[packedB].op.axis
s[packedB].vectorize(z)
s[packedB].parallel(x)

evaluate_operation(s, [A, B, C], target=target, name="mmult", optimization="block caching", log=log)

# Here is the generated IR after write cache blocking.
print(tvm.lower(s, [A, B, C], simple_mode=True))

################################################################################
# 优化 6: Parallelization
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# So far, our computation is only designed to use a single core. Nearly all
# modern processors have multiple cores, and computation can benefit from
# running computations in parallel. The final optimization is to take advantage
# of thread-level parallelization.

# parallel
s[C].parallel(xo)

x, y, z = s[packedB].op.axis
s[packedB].vectorize(z)
s[packedB].parallel(x)

evaluate_operation(
    s, [A, B, C], target=target, name="mmult", optimization="parallelization", log=log
)
 
print(tvm.lower(s, [A, B, C], simple_mode=True))

################################################################################
# Summary of Matrix Multiplication Example
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# After applying the above simple optimizations with only 18 lines of code, our
# generated code can begin to approach the performance of `numpy` with the Math
# Kernel Library (MKL). Since we've been logging the performance as we've been
# working, we can compare the results.

baseline = log[0][1]
print("%s\t%s\t%s" % ("Operator".rjust(20), "Timing".rjust(20), "Performance".rjust(20)))
for result in log:
    print(
        "%s\t%s\t%s"
        % (result[0].rjust(20), str(result[1]).rjust(20), str(result[1] / baseline).rjust(20))
    )
```

## 总结 

上面的Add和MATMUL的示例大致演示了使用TE来定义和手动优化模型的基本步骤：
- 通过TE来描述计算
- 描述Schedule方法：添加所需的优化
- 编译模型
- 按需保存和加载模型
 