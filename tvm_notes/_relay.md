# Relay

Relay是TVM中高级别抽象IR，它提供了多种前端来转换各个深度学习框架的模型

```python

import numpy as np
from tvm import relay
from tvm.relay import testing
import tvm
from tvm import te
from tvm.contrib import graph_executor
import tvm.testing

######################################################################
# (1) 导入模型： 这里我们使用了预编译的resnet_18
batch_size = 1
num_class = 1000
image_shape = (3, 224, 224)
data_shape = (batch_size,) + image_shape
out_shape = (batch_size, num_class)

mod, params = relay.testing.resnet.get_workload(
    num_layers=18, batch_size=batch_size, image_shape=image_shape
)
# set show_meta_data=True if you want to show meta data
print(mod.astext(show_meta_data=False))

######################################################################
# (2) 编译
# ----------- 
# - 优化等级范围 (0 to 3). 
# - 优化passes: operator fusion, pre-computation, layout transformation and so on.
# Relay进行graph-level优化， TVM进行tensor-level优化
#
opt_level = 3
target = tvm.target.Target(target="llvm", host="llvm") #tvm.target.cuda()
with tvm.transform.PassContext(opt_level=opt_level):
    lib = relay.build(mod, target, params=params)

#####################################################################
#（3） 运行
dev = tvm.device(target.kind.name, 0) #tvm.cuda()
data = np.random.uniform(-1, 1, size=data_shape).astype("float32") 
module = graph_executor.GraphModule(lib["default"](dev)) 
module.set_input("data", data) 
module.run() 
out = module.get_output(0, tvm.nd.empty(out_shape)).numpy()

print(out.flatten()[0:10])

######################################################################
# （4） 保存并加载模型
from tvm.contrib import utils

temp = utils.tempdir()
path_lib = temp.relpath("deploy_lib.tar")
lib.export_library(path_lib)
print(temp.listdir())

####################################################

# load the module back.
loaded_lib = tvm.runtime.load_module(path_lib)
input_data = tvm.nd.array(data)

module = graph_executor.GraphModule(loaded_lib["default"](dev))
module.run(data=input_data)
out_deploy = module.get_output(0).numpy()

print(out_deploy.flatten()[0:10])

# 比较结果
tvm.testing.assert_allclose(out_deploy, out, atol=1e-5)

```