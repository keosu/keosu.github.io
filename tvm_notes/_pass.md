# Pass 

## Pass简介  

Pass的概念来源于llvm，一个Pass会在遍历某个IR的时候对其进行某些操作，通常是通过这种途径来进行变换优化。  
同样在TVM中，TVM Pass也是作用于IR进行优化，根据其作用粒度分为：function-level和module-level。   
同时tvm.transform.Sequential能够组合多组Pass。  

## Pass使用示例
```python 
import numpy as np
import tvm
from tvm import te
import tvm.relay as relay
 
# Create  Relay Function
def example():
    shape = (1, 64, 54, 54)
    c_data = np.empty(shape).astype("float32")
    c = relay.const(c_data)
    weight = relay.var("weight", shape=(64, 64, 3, 3))
    x = relay.var("x", relay.TensorType((1, 64, 56, 56), "float32"))
    conv = relay.nn.conv2d(x, weight)
    y = relay.add(c, c)
    y = relay.multiply(y, relay.const(2, "float32"))
    y = relay.add(conv, y)
    z = relay.add(y, c)
    z1 = relay.add(y, c)
    z2 = relay.add(z, z1)
    return relay.Function([x, weight], z2)

 
# 优化
f = example()
mod = tvm.IRModule.from_expr(f)
 
fold_const = relay.transform.FoldConstant() # function-level Pass 
mod = fold_const(mod) 
print(mod)
 
mod = relay.transform.EliminateCommonSubexpr()(mod)
print(mod) 
mod = relay.transform.FuseOps(fuse_opt_level=0)(mod) 
print(mod)

# 使用Sequential创建Passes序列 
# 可以通过opt_level和disabled_pass参数改变其中的某些Pass是否会执行
f = example()
mod = tvm.IRModule.from_expr(f)


seq = tvm.transform.Sequential(
    [
        relay.transform.FoldConstant(),
        relay.transform.EliminateCommonSubexpr(),
        relay.transform.FuseOps(fuse_opt_level=2),
    ]
)
mod1 = seq(mod)
print(mod1)
####
with tvm.transform.PassContext(opt_level=3):
    mod2 = seq(mod)
print(mod2)
 ###
with tvm.transform.PassContext(opt_level=3, disabled_pass=["EliminateCommonSubexpr"]):
    mod3 = seq(mod)
print(mod3)

##############################################################################
# 通过Python装饰器来实现Pass
@relay.transform.function_pass(opt_level=1)
class CustomPipeline:
    """Simple test function to replace one argument to another."""

    def __init__(self, multiplier):
        self.multiplier = multiplier 
    # This function can define a pass.
    def transform_function(self, func, mod, ctx):
        obj = self 
        class ReplaceConstant(tvm.relay.ExprMutator):
            def visit_constant(self, c):
                return relay.multiply(obj.multiplier, c)

        return ReplaceConstant().visit(func)


f = example()
mod = tvm.IRModule.from_expr(f)
custom_pass = CustomPipeline(multiplier=relay.const(3, "float32"))
assert custom_pass.info.name == "CustomPipeline"
mod3 = custom_pass(mod)
print(mod3)

##############################################################################
# 调试Pass：tvm.transform.PrintIR()

f = example()
mod = tvm.IRModule.from_expr(f)
seq = tvm.transform.Sequential(
    [
        relay.transform.FoldConstant(),
        tvm.transform.PrintIR(),
        relay.transform.EliminateCommonSubexpr(),
        relay.transform.FuseOps(),
    ]
)

###############################################################################
# 实现``PassInstrument``来进行更灵活的操作
@tvm.instrument.pass_instrument
class PrintIR:
    """Print the name of the pass, the IR, only before passes execute."""
    def run_before_pass(self, mod, info):
        print("Running pass: {}", info)
        print(mod)

with tvm.transform.PassContext(opt_level=3, instruments=[PrintIR()]):
    with tvm.target.Target("llvm"):
        # Perform the optimizations.
        mod = seq(mod)
print(mod) 
```

## Pass Instrument使用示例

```python
import tvm
import tvm.relay as relay
from tvm.relay.testing import resnet
from tvm.contrib.download import download_testdata
from tvm.relay.build_module import bind_params_by_name
from tvm.ir.instrument import (
    PassTimingInstrument,
    pass_instrument,
)


###############################################################################
# Create An Example Relay Program 
batch_size = 1
num_of_image_class = 1000
image_shape = (3, 224, 224)
output_shape = (batch_size, num_of_image_class)
relay_mod, relay_params = resnet.get_workload(num_layers=18, batch_size=1, image_shape=image_shape)
print("Printing the IR module...")
print(relay_mod.astext(show_meta_data=False))


###############################################################################
# Create PassContext With Instruments 
timing_inst = PassTimingInstrument()
with tvm.transform.PassContext(instruments=[timing_inst]):
    relay_mod = relay.transform.InferType()(relay_mod)
    relay_mod = relay.transform.FoldScaleAxis()(relay_mod)
    # before exiting the context, get profile results.
    profiles = timing_inst.render()
print("Printing results of timing profile...")
print(profiles)


###############################################################################
# Use Current PassContext With Instruments 
cur_pass_ctx = tvm.transform.PassContext.current()
cur_pass_ctx.override_instruments([timing_inst])
relay_mod = relay.transform.InferType()(relay_mod)
relay_mod = relay.transform.FoldScaleAxis()(relay_mod)
profiles = timing_inst.render()
print("Printing results of timing profile...")
print(profiles)


###############################################################################
# Register empty list to clear existing instruments. 
cur_pass_ctx.override_instruments([]) 

###############################################################################
# Create Customized Instrument Class 
@pass_instrument
class RelayCallNodeDiffer:
    def __init__(self):
        self._op_diff = []
        # Passes can be nested.
        # Use stack to make sure we get correct before/after pairs.
        self._op_cnt_before_stack = []

    def enter_pass_ctx(self):
        self._op_diff = []
        self._op_cnt_before_stack = []

    def exit_pass_ctx(self):
        assert len(self._op_cnt_before_stack) == 0, "The stack is not empty. Something wrong."

    def run_before_pass(self, mod, info):
        self._op_cnt_before_stack.append((info.name, self._count_nodes(mod)))

    def run_after_pass(self, mod, info):
        # Pop out the latest recorded pass.
        name_before, op_to_cnt_before = self._op_cnt_before_stack.pop()
        assert name_before == info.name, "name_before: {}, info.name: {} doesn't match".format(
            name_before, info.name
        )
        cur_depth = len(self._op_cnt_before_stack)
        op_to_cnt_after = self._count_nodes(mod)
        op_diff = self._diff(op_to_cnt_after, op_to_cnt_before)
        # only record passes causing differences.
        if op_diff:
            self._op_diff.append((cur_depth, info.name, op_diff))

    def get_pass_to_op_diff(self):
        """
        return [
          (depth, pass_name, {op_name: diff_num, ...}), ...
        ]
        """
        return self._op_diff

    @staticmethod
    def _count_nodes(mod):
        """Count the number of occurrences of each operator in the module"""
        ret = {}

        def visit(node):
            if isinstance(node, relay.expr.Call):
                if hasattr(node.op, "name"):
                    op_name = node.op.name
                else:
                    # Some CallNode may not have 'name' such as relay.Function
                    return
                ret[op_name] = ret.get(op_name, 0) + 1

        relay.analysis.post_order_visit(mod["main"], visit)
        return ret

    @staticmethod
    def _diff(d_after, d_before):
        """Calculate the difference of two dictionary along their keys.
        The result is values in d_after minus values in d_before.
        """
        ret = {}
        key_after, key_before = set(d_after), set(d_before)
        for k in key_before & key_after:
            tmp = d_after[k] - d_before[k]
            if tmp:
                ret[k] = d_after[k] - d_before[k]
        for k in key_after - key_before:
            ret[k] = d_after[k]
        for k in key_before - key_after:
            ret[k] = -d_before[k]
        return ret


###############################################################################
# Apply Passes and Multiple Instrument Classes 
call_node_inst = RelayCallNodeDiffer()
desired_layouts = {
    "nn.conv2d": ["NHWC", "HWIO"],
}
pass_seq = tvm.transform.Sequential(
    [
        relay.transform.FoldConstant(),
        relay.transform.ConvertLayout(desired_layouts),
        relay.transform.FoldConstant(),
    ]
)
relay_mod["main"] = bind_params_by_name(relay_mod["main"], relay_params)
# timing_inst is put after call_node_inst.
# So the execution time of ``call_node.inst.run_after_pass()`` is also counted.
with tvm.transform.PassContext(opt_level=3, instruments=[call_node_inst, timing_inst]):
    relay_mod = pass_seq(relay_mod)
    profiles = timing_inst.render()
# Uncomment the next line to see timing-profile results.
# print(profiles)

```
## Pass总结
基本使用方式：  
1. 使用TVM自带的Pass：TVM.relay.transform
2. 使用Sequential组合多个Pass的序列
3. 使用装饰器来创建自定义Pass: @relay.transform.function_pass(opt_level=1)

调试Pass的方法： 
1. 通过tvm.transform.PrintIR()打印信息
2. 通过装饰器自定义访问行为： @tvm.instrument.pass_instrument