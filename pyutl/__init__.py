import inspect
from types import ModuleType
from typing import TypeVar, Generic, Optional, Annotated, get_origin, get_args, Union, ForwardRef, Any

from ._pyutl import *

class TLSection:
    TYPES = 0
    FUNCTIONS = 1


T = TypeVar("T", bound=TLType)
TAny = TypeVar("TAny")


# TODO: rename to _TLFieldDef
class _TLType:
    NAME: str | None


class TLFlags(_TLType):
    NAME = "#"


class TLInt(_TLType):
    NAME = "int"


class TLLong(_TLType):
    NAME = "long"


class TLInt128(_TLType):
    NAME = "int128"


class TLInt256(_TLType):
    NAME = "int256"


class TLFloat(_TLType):
    NAME = "double"


class TLBool(_TLType):
    NAME = "Bool"


class TLTrue(_TLType):
    NAME = "true"


class TLString(_TLType):
    NAME = "string"


class TLBytes(_TLType):
    NAME = "bytes"


class TLObj(_TLType):
    NAME = None

    __slots__ = ("name",)

    def __init__(self, typ: str | type[TLObject]) -> None:
        if isinstance(typ, type) and issubclass(typ, TLObject):
            raise NotImplementedError("Passing tl types to pyutl.TLObject is not supported yet")
        self.name = typ


class TLVec(_TLType):
    NAME = None

    __slots__ = ("inner_type",)

    def __init__(self, inner_type: _TLType) -> None:
        if isinstance(inner_type, TLOpt):
            raise ValueError("pyutl.TLVec can't contain pyutl.TLOpt")
        if isinstance(inner_type, TLTrue):
            raise ValueError("pyutl.TLVec can't contain pyutl.TLTrue")
        self.inner_type = inner_type


class TLOpt(_TLType):
    NAME = None

    __slots__ = ("inner_type", "bit", "flags_num",)

    def __init__(self, inner_type: _TLType, bit: int, flags_num: int | None = None) -> None:
        if isinstance(inner_type, TLOpt):
            raise ValueError("pyutl.TLOpt can't contain another pyutl.TLOpt")
        self.inner_type = inner_type
        self.bit = bit
        self.flags_num = flags_num


class _FieldAccumulatorDict(dict):
    def __init__(self, fields: list | None = None):
        if fields is None:
            fields = []
            super().__setitem__("__fields", fields)
        self.__fields = fields
        super().__init__()

    def __setitem__(self, key: str, value: Any) -> None:
        if not key.startswith("__") and isinstance(value, _TLType):
            self.__fields.append((key, value))
        super().__setitem__(key, value)


def _resolve_field_type(field_def: _TLType, flags_count: int = 0) -> str:
    if field_def.NAME is not None:
        return field_def.NAME

    if isinstance(field_def, TLObj):
        return field_def.name
    if isinstance(field_def, TLVec):
        return f"vector<{_resolve_field_type(field_def.inner_type)}>"
    if isinstance(field_def, TLOpt):
        flags_num = field_def.flags_num or flags_count
        return f"flags{flags_num if flags_num > 1 else ''}.{field_def.bit}?{_resolve_field_type(field_def.inner_type)}"

    raise RuntimeError("Unreachable")


class _AnnotatedTLObjectMeta(type):
    def __prepare__(meta, *args, **kwargs):
        return _FieldAccumulatorDict()

    def __new__(meta, name, bases, class_dict) -> type[TLObject]:
        if class_dict.get("__tl__"):
            tl_obj_type = parse_tl(class_dict["__tl__"], class_dict["__layer__"], class_dict.get("__section__", TLSection.TYPES))
            setattr(tl_obj_type, "__tl__", class_dict["__tl__"])
            return tl_obj_type

        orig_bases = class_dict.get("__orig_bases__")
        if orig_bases and get_origin(orig_bases[0]) is Generic:
            return type.__new__(meta, name, bases, class_dict)

        if not orig_bases or get_origin(orig_bases[0]) is not AnnotatedTLObject \
                or not (base_args := get_args(orig_bases[0])):
            raise ValueError(f"Class {name} has invalid __orig_bases__")

        if isinstance(base_args[0], type) and issubclass(base_args[0], TLType):
            base_type = base_args[0].__name__
        elif isinstance(base_args[0], ForwardRef):
            base_type = base_args[0].__forward_arg__
        else:
            raise ValueError(f"Class {name} has invalid __orig_bases__")

        tl_def = f"{name}#{hex(class_dict['__tl_id__'])[2:]}"

        fields: list[tuple[str, _TLType]] = class_dict["__fields"]
        flags_count = 0
        for field_name, field_def in fields:
            flags_count += isinstance(field_def, TLFlags)
            tl_type = _resolve_field_type(field_def, flags_count)
            tl_def += f" {field_name}:{tl_type}"

        tl_def += f" = {base_type};"

        tl_obj_type = parse_tl(tl_def, class_dict["__layer__"], class_dict.get("__section__", TLSection.TYPES))
        setattr(tl_obj_type, "__tl__", tl_def)

        return tl_obj_type


class AnnotatedTLObject(Generic[T], TLObject, metaclass=_AnnotatedTLObjectMeta):
    ...
