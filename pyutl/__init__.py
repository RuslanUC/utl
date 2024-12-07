import inspect
from types import ModuleType
from typing import TypeVar, Generic, Optional, Annotated, get_origin, get_args, Union, ForwardRef

from ._pyutl import *

class TLSection:
    TYPES = 0
    FUNCTIONS = 1


T = TypeVar("T", bound=TLType)
TAny = TypeVar("TAny")


class TLFlags(int):
    ...


class TLInt(int):
    ...


class TLLong(int):
    ...


class TLInt128(int):
    ...


class TLInt256(int):
    ...


class _TLOptionalMeta(type):
    def __getitem__(self, params: tuple[type[TAny], int] | tuple[type[TAny], int, int] | tuple[str, int] | tuple[str, int, int] | int | tuple[int, int]):
        if isinstance(params, int):
            params = (params,)

        is_true = self is TLTrue
        base_args = 1 if is_true else 2

        if not isinstance(params, tuple) or len(params) < base_args or len(params) > (base_args + 1):
            raise TypeError(f"{self.__name__}[...] should be used with {base_args} or {base_args + 1} arguments.")

        if len(params) == base_args:
            metadata = (params[base_args - 1], 1)
        else:
            metadata = (params[base_args - 1], params[base_args])

        if is_true:
            return Annotated[bool, metadata]
        else:
            origin = params[0]
            return Annotated[Optional[origin], metadata]


class TLOptional(Generic[TAny], metaclass=_TLOptionalMeta):
    pass


class TLTrue(Generic[TAny], metaclass=_TLOptionalMeta):
    pass


class _FieldAccumulatorDict(dict):
    def __init__(self, fields: list | None = None):
        if fields is None:
            fields = []
            super().__setitem__("__fields", fields)
        self.__fields = fields
        super().__init__()

    def __setitem__(self, key, value):
        if not key.startswith("__"):
            self.__fields.append(key)
        elif key == "__annotations__":
            value = _FieldAccumulatorDict(self.__fields)
        super().__setitem__(key, value)


_type_to_tl = {
    TLInt: "int",
    TLLong: "long",
    TLInt128: "int128",
    TLInt256: "int256",
    TLFlags: "#",
    float: "double",
    str: "string",
    bytes: "bytes",
    bool: "Bool",
}


def _resolve_annotation(annotation) -> str:
    tl_type = _type_to_tl.get(annotation)
    if tl_type is not None:
        return tl_type
    elif isinstance(annotation, type) and issubclass(annotation, TLType):
       return annotation.__name__
    elif get_origin(annotation) is list:
        return f"vector<{_resolve_annotation(get_args(annotation)[0])}>"
    elif get_origin(annotation) is Annotated:
        args = get_args(annotation)
        flag_bit, flag_num = args[1]
        if args[0] is bool:
            resolved = "true"
        else:
            arg = get_args(args[0])[0]
            if isinstance(arg, ForwardRef):
                resolved = arg.__forward_arg__
            else:
                resolved = _resolve_annotation(arg)
        return f"flags{flag_num if flag_num > 1 else ''}.{flag_bit}?{resolved}"


class _AnnotatedTLObjectMeta(type):
    def __prepare__(meta, *args, **kwargs):
        return _FieldAccumulatorDict()

    def __new__(meta, name, bases, class_dict) -> type[TLObject]:
        if class_dict.get("__tl__"):
            tl_obj_type = parse_tl(class_dict["__tl__"], class_dict["__layer__"], class_dict.get("__section__", TLSection.TYPES))
            setattr(tl_obj_type, "__tl__", class_dict["__tl__"])
            return tl_obj_type

        if "__annotations__" not in class_dict:
            class_dict["__annotations__"] = {}

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

        tmp_module = ModuleType("__tmp__")
        setattr(tmp_module, "__annotations__", class_dict["__annotations__"])
        annotations = inspect.get_annotations(tmp_module, eval_str=True)

        tl_def = f"{name}#{hex(class_dict['__tl_id__'])[2:]}"

        for field_name in class_dict["__fields"]:
            annotation = annotations[field_name]
            tl_type = _resolve_annotation(annotation)
            if tl_type is None:
                raise ValueError(f"Failed to resolve field {field_name} in class {name}!")

            tl_def += f" {field_name}:{tl_type}"

        tl_def += f" = {base_type};"

        tl_obj_type = parse_tl(tl_def, class_dict["__layer__"], class_dict.get("__section__", TLSection.TYPES))
        setattr(tl_obj_type, "__tl__", tl_def)

        return tl_obj_type


class AnnotatedTLObject(Generic[T], TLObject, metaclass=_AnnotatedTLObjectMeta):
    ...
