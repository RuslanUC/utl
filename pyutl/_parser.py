from __future__ import annotations

from enum import Enum

from pyutl import TLSection


class FieldType(Enum):
    INT32 = 0
    FLAGS = 1
    INT64 = 2
    INT128 = 3
    INT256 = 4
    DOUBLE = 5
    FULL_BOOL = 6
    BIT_BOOL = 7
    BYTES = 8
    STRING = 9
    TLOBJECT = 10
    VECTOR = 11


_typestr_to_field_type = {
    "int": FieldType.INT32,
    "long": FieldType.INT64,
    "int128": FieldType.INT128,
    "int256": FieldType.INT256,
    "double": FieldType.DOUBLE,
    "Bool": FieldType.FULL_BOOL,
    "true": FieldType.BIT_BOOL,
    "bytes": FieldType.BYTES,
    "string": FieldType.STRING,
    "!X": FieldType.TLOBJECT,
    "TLObject": FieldType.TLOBJECT,
}
_field_type_to_typestr = {
    FieldType.FLAGS: "#",
    FieldType.INT32: "int",
    FieldType.INT64: "long",
    FieldType.INT128: "int128",
    FieldType.INT256: "int256",
    FieldType.DOUBLE: "double",
    FieldType.FULL_BOOL: "Bool",
    FieldType.BIT_BOOL: "true",
    FieldType.BYTES: "bytes",
    FieldType.STRING: "string",
}
_field_type_to_pytype = {
    FieldType.FLAGS: "pyutl.TLFlags",
    FieldType.INT32: "pyutl.TLInt",
    FieldType.INT64: "pyutl.TLLong",
    FieldType.INT128: "pyutl.TLInt128",
    FieldType.INT256: "pyutl.TLInt256",
    FieldType.DOUBLE: "float",
    FieldType.FULL_BOOL: "bool",
    FieldType.BYTES: "bytes",
    FieldType.STRING: "str",
}


class SlotsRepr:
    __slots__ = ()

    def __repr__(self) -> str:
        fields = ", ".join([f"{slot}={getattr(self, slot)!r}" for slot in self.__slots__])
        return f"{self.__class__.__name__}({fields})"


class VectorDef(SlotsRepr):
    __slots__ = ("type", "sub_def",)

    def __init__(self, type_: FieldType, sub_def: str | VectorDef | None):
        self.type = type_
        self.sub_def = sub_def

    def to_tl(self) -> str:
        inner = _field_type_to_typestr.get(self.type)
        if self.type == FieldType.VECTOR or (self.type == FieldType.TLOBJECT and self.sub_def is not None):
            inner = self.sub_def.to_tl() if isinstance(self.sub_def, VectorDef) else self.sub_def
        elif self.type == FieldType.TLOBJECT and self.sub_def is None:
            inner = "TLObject"

        return f"vector<{inner}>"

    def to_python(self) -> str:
        inner = _field_type_to_pytype.get(self.type)
        if self.type == FieldType.VECTOR or (self.type == FieldType.TLOBJECT and self.sub_def is not None):
            inner = self.sub_def.to_python() if isinstance(self.sub_def, VectorDef) else f"\"{self.sub_def}\""
        elif self.type == FieldType.TLOBJECT and self.sub_def is None:
            inner = "\"TLObject\""

        return f"list[{inner}]"


class FieldDef(SlotsRepr):
    __slots__ = ("name", "type", "flag_num", "flag_bit", "sub_def",)

    def __init__(self, name: str, type_: FieldType, flag_num: int, flag_bit: int, sub_def: str | VectorDef | None):
        self.name = name
        self.type = type_
        self.flag_num = flag_num
        self.flag_bit = flag_bit
        self.sub_def = sub_def

    def to_tl(self) -> str:
        real_type = _field_type_to_typestr.get(self.type)
        if self.type == FieldType.VECTOR or (self.type == FieldType.TLOBJECT and self.sub_def is not None):
            real_type = self.sub_def.to_tl() if isinstance(self.sub_def, VectorDef) else self.sub_def
        elif self.type == FieldType.TLOBJECT and self.sub_def is None:
            real_type = "TLObject"

        flags_part = ""
        if self.type != FieldType.FLAGS and self.flag_num:
            flags_part = f"flags{self.flag_num if self.flag_num > 1 else ''}.{self.flag_bit}?"

        return f"{self.name}:{flags_part}{real_type}"

    def to_python(self) -> str:
        real_type = _field_type_to_pytype.get(self.type)
        if self.type == FieldType.VECTOR or (self.type == FieldType.TLOBJECT and self.sub_def is not None):
            real_type = self.sub_def.to_python() if isinstance(self.sub_def, VectorDef) else f"\"{self.sub_def}\""
        elif self.type == FieldType.TLOBJECT and self.sub_def is None:
            real_type = "\"TLObject\""

        if self.type != FieldType.FLAGS and self.type != FieldType.BIT_BOOL and self.flag_num:
            flag_num = "" if self.flag_num == 1 else f", {self.flag_num}"
            real_type = f"pyutl.TLOptional[{real_type}, {self.flag_bit}{flag_num}]"
        elif self.type != FieldType.FLAGS and self.type == FieldType.BIT_BOOL and self.flag_num:
            flag_num = "" if self.flag_num == 1 else f", {self.flag_num}"
            real_type = f"pyutl.TLTrue[{self.flag_bit}{flag_num}]"

        return f"{self.name}: {real_type}"


class MessageDef(SlotsRepr):
    __slots__ = ("id", "name", "namespace", "type", "layer", "section", "fields",)
    
    def __init__(self, id_: int, name: str, namespace: str, type_: str, layer: int, section: int, fields: list[FieldDef]):
        self.id = id_
        self.name = name
        self.namespace = namespace
        self.type = type_
        self.layer = layer
        self.section = section
        self.fields = fields

    def full_name(self, sep: str = ".") -> str:
        return f"{self.namespace}{sep}{self.name}" if self.namespace else self.name

    def to_tl(self) -> str:
        fields = " ".join([field.to_tl() for field in self.fields])
        if fields:
            fields += " "

        return f"{self.full_name()}#{hex(self.id)[2:]} {fields}= {self.type};"

    def to_python_class(self) -> str:
        obj_type = self.type
        if obj_type.lower().startswith("vector<"):
            obj_type = f"vector_{obj_type[7:-1]}"
        result = [
            f"class {self.full_name('_')}(pyutl.AnnotatedTLObject[\"{obj_type}\"]):",
            f"    __tl_id__ = {hex(self.id)}",
            f"    __layer__ = {self.layer}",
            f"    __section__ = {'pyutl.TLSection.TYPES' if self.section == 0 else 'pyutl.TLSection.FUNCTIONS'}",
            f"    __tl__ = \"{self.to_tl()}\"",
            f"",
            *[f"    {field.to_python()}" for field in self.fields],
            f"",
            f"",
        ]

        if self.fields:
            result.append("")

        return "\n".join(result)


def _resolve_field_type(field: FieldDef | VectorDef, field_type: str) -> bool:
    if field_type.lower().startswith("vector<"):
        field_type = field_type[6:]
        if field_type[-1] != ">":
            return False
        field.type = FieldType.VECTOR
        field.sub_def = VectorDef(None, None)
        return _resolve_field_type(field.sub_def, field_type[1:-1])

    if field_type == "#":
        field.type = FieldType.FLAGS
        if len(field.name) > 5:
            field.flag_num = int(field.name[-1])
        else:
            field.flag_num = 1

        return True

    if field_type in _typestr_to_field_type:
        field.type = _typestr_to_field_type[field_type]
        return True

    field.type = FieldType.TLOBJECT
    field.sub_def = field_type

    return True


def parse_line(line: str, layer: int = 1, section: int = TLSection.TYPES) -> MessageDef | None:
    name_end = line.find("#")
    if name_end == -1:
        return

    name, line = line[:name_end], line[name_end + 1:]
    name_with_ns = name.split(".", maxsplit=1)
    if len(name_with_ns) == 1:
        name, namespace = name_with_ns[0], None
    else:
        namespace, name = name_with_ns

    id_end = line.find(" ")
    if id_end == -1:
        return

    tl_id, line = int(line[:id_end], 16), line[id_end + 1:]
    fields_end = line.find("=")
    if fields_end == -1:
        return

    fields, line = line[:fields_end], line[fields_end + 1:]
    fields = fields.strip()
    if not line.endswith(";") or not (type_name := line[:-1].strip()):
        return

    message_def = MessageDef(
        id_=tl_id,
        name=name,
        namespace=namespace,
        type_=type_name,
        layer=layer,
        section=section,
        fields=[],
    )

    for field in fields.split(" "):
        field = field.strip()
        if not field or field.startswith("{") or field.endswith("}"):
            continue

        field_name, field_type = field.split(":")
        if not field_name or not field_type:
            return

        if field_name in ("self", "from", "private", "public"):
            field_name = f"{field_name}_"

        flag_num = 0
        flag_bit = 0
        if "?" in field_type:
            flag_info, field_type = field_type.split("?")
            flag_field, flag_bit = flag_info.split(".")
            flag_num = int(flag_field[-1]) if len(flag_field) > 5 else 1
            flag_bit = int(flag_bit)

        message_def.fields.append(field_def := FieldDef(
            name=field_name,
            type_=None,
            flag_num=flag_num,
            flag_bit=flag_bit,
            sub_def=None,
        ))

        if not _resolve_field_type(field_def, field_type):
            return

    return message_def
