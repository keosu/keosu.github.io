import os
import typing
import json 

class BetterSerializable:
    def __init__(self, *args):
        self.args = args
    def __init_subclass__(cls) -> None:
        super().__init_subclass__()
        register_class(cls)
    def serialize(self):
        return json.dumps({
            "class": self.__class__.__name__,
            "args": self.args,
        })
    def __repr__(self):
        name = self.__class__.__name__
        args_str = ", ".join(str(x) for x in self.args)
        return f"{name}({args_str})"
 
 
registry = {}


def register_class(target_class):
    registry[target_class.__name__] = target_class

def deserialize(data):
    params = json.loads(data)
    name = params["class"]
    target_class = registry[name]
    return target_class(*params["args"])

class Meta(type):
    def __new__(meta, name, bases, class_dict):
        cls = type.__new__(meta, name, bases, class_dict)
        register_class(cls)
        return cls


class RegisteredSerializable(BetterSerializable, metaclass=Meta):
    pass
class DerivedCls(BetterSerializable):
  pass

print('-----')