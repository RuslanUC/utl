# uTL

A small Telegram [Type Language](https://core.telegram.org/mtproto/TL) reflection-based implementation in C heavily inspired by Google's [upb](https://github.com/protocolbuffers/protobuf/tree/main/upb).

> [!CAUTION]
> This project is in development and (since it is also my one of the first python c extensions without using libraries like nanobind) may contain critical bugs such as memory leaks. Use at your own risk!

# Installation
```shell
pip install <TODO: package on PyPI>
```

Or install from git:
```shell
pip install git+https://github.com/RuslanUC/utl
```

# Quick start
### TL object definition, serialization, deserialization
```python
from io import BytesIO
import pyutl

SomeConstructor = pyutl.parse_tl("SomeConstructor#12345678 some_string_field:string = SomeType;", 1, pyutl.TLSection.TYPES)
obj = SomeConstructor(some_string_field="this is a string in SomeConstructor object!")
print(obj)
# SomeConstructor(some_string_field="this is a string in SomeConstructor object!")
serialized = obj.write()
print(obj)
# b"..."

# Skipping first 4 bytes because it is tl id, and it is not needed when .read_bytes is called on known type
new_obj = SomeConstructor.read_bytes(serialized[4:])
# or
new_obj2 = SomeConstructor.read(BytesIO(serialized[4:]))
# or
# Here we are not skipping first 4 bytes since we are deserializing object with TLObject (type is unknown)
new_obj3 = pyutl.TLObject.read_bytes(serialized)
# or
new_obj4 = pyutl.TLObject.read(BytesIO(serialized))

print(new_obj)
# SomeConstructor(some_string_field="this is a string in SomeConstructor object!")

print(obj == new_obj)
# True
print(obj == new_obj2)
# True
print(obj == new_obj3)
# True
print(obj == new_obj4)
# True
```

### TL object definition via pyutl.parse_tl
You can define tl object type with tl definition string like this:
```python
import pyutl
SomeConstructor = pyutl.parse_tl("SomeConstructor#12345678 some_string_field:string = SomeType;", 1, pyutl.TLSection.TYPES)
```

### TL object definition via python type-annotated class
Also, you can define tl object using python type-annotated class:
```python
import pyutl

# TL type inside AnnotatedTLObject[...] may be string or TLType subclass that you got from pyutl.create_type or pyutl.get_type
class SomeConstructor(pyutl.AnnotatedTLObject["SomeType"]):
    __tl_id__ = 0x12345678
    __layer__ = 1
    __section__ = pyutl.TLSection.TYPES  # Optional, pyutl.TLSection.TYPES by default
    # Optional, may be tl definition to skip annotations parsing,
    # if None - then after parsing it will be set to generated tl definition so you can set or reuse it later
    __tl__ = None

    some_string_field: str
```
