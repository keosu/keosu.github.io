# TOPI

TVM Operator Inventory (TOPI)提供了numpy风格的高级抽象算子.  

```python 
from __future__ import absolute_import, print_function

import tvm
import tvm.testing
from tvm import te
from tvm import topi
import numpy as np

######################################################################
# 基础实例：按行求和(等价numpy代码:`B = numpy.sum(A, axis=1)`) 
n = te.var("n")
m = te.var("m")
A = te.placeholder((n, m), name="A")
k = te.reduce_axis((0, m), "k")
B = te.compute((n,), lambda i: te.sum(A[i, k], axis=k), name="B")
s = te.create_schedule(B.op)

####### 
# 查看生成的IR
print(tvm.lower(s, [A], simple_mode=True))

#######
# 使用预定义topi方法
C = topi.sum(A, axis=1)
ts = te.create_schedule(C.op)
print(tvm.lower(ts, [A], simple_mode=True))

######################################################################
# Numpy-style 广播操作
x, y = 100, 10
a = te.placeholder((x, y, y), name="a")
b = te.placeholder((y, y), name="b")
c = a + b  # same as topi.broadcast_add
d = a * b  # same as topi.broadcast_mul

######################################################################
# TOPI定义了针对不同平台的调度器，如CUDA，x86... 
e = topi.elemwise_sum([c, d])
f = e / 2.0
g = topi.sum(f)
with tvm.target.Target(target="llvm", host="llvm"):
    # sg = topi.cuda.schedule_reduce(g) 
    sg = topi.x86.schedule_reduce(g)
    print(tvm.lower(sg, [a, b], simple_mode=True))

# examine schedule stage
print(sg.stages)

###### 
# 和numpy运算结果做比较
func = tvm.build(sg, [a, b, g], "llvm")
dev = tvm.device("llvm", 0)
a_np = np.random.uniform(size=(x, y, y)).astype(a.dtype)
b_np = np.random.uniform(size=(y, y)).astype(b.dtype)
g_np = np.sum(np.add(a_np + b_np, a_np * b_np) / 2.0)
a_nd = tvm.nd.array(a_np, dev)
b_nd = tvm.nd.array(b_np, dev)
g_nd = tvm.nd.array(np.zeros(g_np.shape, dtype=g_np.dtype), dev)
func(a_nd, b_nd, g_nd)
tvm.testing.assert_allclose(g_nd.numpy(), g_np, rtol=1e-5)

######################################################################
# TOPI 提供了常用的神经网络算子，如softmax，conv
######################################################################
# NOTE: TOPI的实现因后端而异，目标平台和调度器必须一致 

tarray = te.placeholder((512, 512), name="tarray")
softmax_topi = topi.nn.softmax(tarray)
with tvm.target.Target(target="llvm", host="llvm"):
    # sst = topi.cuda.schedule_softmax(softmax_topi)
    sst = topi.x86.schedule_softmax(softmax_topi) 
    print(tvm.lower(sst, [tarray], simple_mode=True))


data = te.placeholder((1, 3, 224, 224))
kernel = te.placeholder((10, 3, 5, 5))

with tvm.target.Target(target="llvm", host="llvm"): 
    conv = topi.nn.conv2d_nchw(data, kernel, 1, 2, 1)
    out = topi.nn.relu(conv)
    # sconv = topi.cuda.schedule_conv2d_nchw([out])
    sconv = topi.x86.schedule_conv2d_nchw([out]) 
    print(tvm.lower(sconv, [data, kernel], simple_mode=True))

```