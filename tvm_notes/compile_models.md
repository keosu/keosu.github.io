# TVM编译模型

# Pytorch 模型

```python 
import tvm
from tvm import relay 
import numpy as np 
from tvm.contrib.download import download_testdata 
import torch
import torchvision
######################################################################
# 获取 pytorch模型 
model_name = "resnet18"
model = getattr(torchvision.models, model_name)(pretrained=True)
model = model.eval() 
# We grab the TorchScripted model via tracing
input_shape = [1, 3, 224, 224]
input_data = torch.randn(input_shape)
scripted_model = torch.jit.trace(model, input_data).eval()

######################################################################
# 获取测试图片
from PIL import Image 
img_url = "https://github.com/dmlc/mxnet.js/blob/main/data/cat.png?raw=true"
img_path = download_testdata(img_url, "cat.png", module="data")
img = Image.open(img_path).resize((224, 224))

# Preprocess the image and convert to tensor
from torchvision import transforms 
my_preprocess = transforms.Compose(
    [
        transforms.Resize(256),
        transforms.CenterCrop(224),
        transforms.ToTensor(),
        transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
    ]
)
img = my_preprocess(img)
img = np.expand_dims(img, 0)

######################################################################
# 使用TVM编译运行
input_name = "input0"
shape_list = [(input_name, img.shape)]
mod, params = relay.frontend.from_pytorch(scripted_model, shape_list) 

# Relay Build 
target = tvm.target.Target("llvm", host="llvm")
dev = tvm.cpu(0)
with tvm.transform.PassContext(opt_level=3):
    lib = relay.build(mod, target=target, params=params) 

from tvm.contrib import graph_executor 
dtype = "float32"
m = graph_executor.GraphModule(lib["default"](dev)) 
m.set_input(input_name, tvm.nd.array(img.astype(dtype))) 
m.run() 
tvm_output = m.get_output(0) 
top1_tvm = np.argmax(tvm_output.numpy()[0]) 

######################################################################
# 使用 pytorch运行并对比结果
with torch.no_grad():
    torch_img = torch.from_numpy(img)
    output = model(torch_img)  
    top1_torch = np.argmax(output.numpy()) 

print("Relay top-1 id: {}".format(top1_tvm))
print("Torch top-1 id: {}".format(top1_torch))

```

# TensorFLow模型

```python
 import tvm
from tvm import te
from tvm import relay 
import numpy as np
import os.path
 
import tensorflow as tf 
try:
    tf_compat_v1 = tf.compat.v1
except ImportError:
    tf_compat_v1 = tf

# Tensorflow utility functions
import tvm.relay.testing.tf as tf_testing
 
repo_base = "https://github.com/dmlc/web-data/raw/main/tensorflow/models/InceptionV1/"  
img_name = "elephant-299.jpg"
image_url = os.path.join(repo_base, img_name)

model_name = "classify_image_graph_def-with_shapes.pb"
model_url = os.path.join(repo_base, model_name) 
# Image label map
map_proto = "imagenet_2012_challenge_label_map_proto.pbtxt"
map_proto_url = os.path.join(repo_base, map_proto) 
# Human readable text for labels
label_map = "imagenet_synset_to_human_label_map.txt"
label_map_url = os.path.join(repo_base, label_map)

######################################################################
# Download required files 
from tvm.contrib.download import download_testdata

img_path = download_testdata(image_url, img_name, module="data")
model_path = download_testdata(model_url, model_name, module=["tf", "InceptionV1"])
map_proto_path = download_testdata(map_proto_url, map_proto, module="data")
label_path = download_testdata(label_map_url, label_map, module="data")

######################################################################
# tensorflow导入模型

with tf_compat_v1.gfile.GFile(model_path, "rb") as f:
    graph_def = tf_compat_v1.GraphDef()
    graph_def.ParseFromString(f.read())
    graph = tf.import_graph_def(graph_def, name="")
    # Call the utility to import the graph definition into default graph.
    graph_def = tf_testing.ProcessGraphDefParam(graph_def)
    # Add shapes to the graph.
    with tf_compat_v1.Session() as sess:
        graph_def = tf_testing.AddShapesToGraphDef(sess, "softmax")

######################################################################
# 使用TVM导入tf模型并运行

from PIL import Image 
image = Image.open(img_path).resize((299, 299)) 
x = np.array(image)

target = tvm.target.Target("llvm", host="llvm")
layout = None
dev = tvm.cpu(0)

shape_dict = {"DecodeJpeg/contents": x.shape}
dtype_dict = {"DecodeJpeg/contents": "uint8"}
mod, params = relay.frontend.from_tensorflow(graph_def, layout=layout, shape=shape_dict)
 
with tvm.transform.PassContext(opt_level=3):
    lib = relay.build(mod, target, params=params)

from tvm.contrib import graph_executor 
dtype = "uint8"
m = graph_executor.GraphModule(lib["default"](dev)) 
m.set_input("DecodeJpeg/contents", tvm.nd.array(x.astype(dtype))) 
m.run() 
tvm_output = m.get_output(0, tvm.nd.empty(((1, 1008)), "float32"))

######################################################################
# 显示TVM结果
predictions = tvm_output.numpy()
predictions = np.squeeze(predictions)
 
node_lookup = tf_testing.NodeLookup(label_lookup_path=map_proto_path, uid_lookup_path=label_path)
 
top_k = predictions.argsort()[-5:][::-1]
for node_id in top_k:
    human_string = node_lookup.id_to_string(node_id)
    score = predictions[node_id]
    print("%s (score = %.5f)" % (human_string, score))

######################################################################
# 使用tensorflow推理并显示结果 
def create_graph():
    """Creates a graph from saved GraphDef file and returns a saver."""
    # Creates graph from saved graph_def.pb.
    with tf_compat_v1.gfile.GFile(model_path, "rb") as f:
        graph_def = tf_compat_v1.GraphDef()
        graph_def.ParseFromString(f.read())
        graph = tf.import_graph_def(graph_def, name="")
        # Call the utility to import the graph definition into default graph.
        graph_def = tf_testing.ProcessGraphDefParam(graph_def)


def run_inference_on_image(image): 
    if not tf_compat_v1.gfile.Exists(image):
        tf.logging.fatal("File does not exist %s", image)
    image_data = tf_compat_v1.gfile.GFile(image, "rb").read()

    # Creates graph from saved GraphDef.
    create_graph()

    with tf_compat_v1.Session() as sess:
        softmax_tensor = sess.graph.get_tensor_by_name("softmax:0")
        predictions = sess.run(softmax_tensor, {"DecodeJpeg/contents:0": image_data})

        predictions = np.squeeze(predictions)
 
        node_lookup = tf_testing.NodeLookup(
            label_lookup_path=map_proto_path, uid_lookup_path=label_path
        )

        # Print top 5 predictions from tensorflow.
        top_k = predictions.argsort()[-5:][::-1]
        print("===== TENSORFLOW RESULTS =======")
        for node_id in top_k:
            human_string = node_lookup.id_to_string(node_id)
            score = predictions[node_id]
            print("%s (score = %.5f)" % (human_string, score))

run_inference_on_image(img_path)

```


# 总结

```python
target = "llvm" 
input_name = "1"
shape_dict = {input_name: x.shape}
mod, params = relay.frontend.from_onnx(onnx_model, shape_dict) 
with tvm.transform.PassContext(opt_level=1):
    executor = relay.build_module.create_executor(
        "graph", mod, tvm.cpu(0), target, params
    ).evaluate()
 
dtype = "float32"
tvm_output = executor(tvm.nd.array(x.astype(dtype))).numpy()

# 使用TVM编译运行 
mod, params = relay.frontend.from_pytorch(scripted_model, shape_list) # [{name:shape}]  
target = tvm.target.Target("llvm", host="llvm")
dev = tvm.cpu(0)
with tvm.transform.PassContext(opt_level=3):
    lib = relay.build(mod, target=target, params=params)  
dtype = "float32"
m = graph_executor.GraphModule(lib["default"](dev)) 
m.set_input(input_name, tvm.nd.array(img.astype(dtype))) 
m.run() 
tvm_output = m.get_output(0) 

# tensorflow
mod, params = relay.frontend.from_tensorflow(graph_def, layout=layout, shape=shape_dict) 
with tvm.transform.PassContext(opt_level=3):
    lib = relay.build(mod, target, params=params) 
dtype = "uint8"
m = graph_executor.GraphModule(lib["default"](dev)) 
m.set_input("DecodeJpeg/contents", tvm.nd.array(x.astype(dtype))) 
m.run() 
tvm_output = m.get_output(0, tvm.nd.empty(((1, 1008)), "float32"))


target = "llvm" 
with tvm.transform.PassContext(opt_level=3, disabled_pass=["FoldScaleAxis"]):
    vm_exec = relay.vm.compile(mod, target=target, params=params)

######################################################################
# Inference with Relay VM
# -----------------------
dev = tvm.cpu()
vm = VirtualMachine(vm_exec, dev)
vm.set_input("main", **{input_name: img})
tvm_res = vm.run()

```