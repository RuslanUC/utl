import os
from io import BytesIO
from typing import Annotated

import pytest

print(f"\nPid: {os.getpid()}")
input("Press enter to continue...")

import pyutl

SKIP_TESTS = 0


@pytest.mark.skipif(SKIP_TESTS >= 1, reason="")
def test_parse() -> None:
    assert pyutl.def_pool

    InputGeoPointBase = pyutl.create_type("InputGeoPoint")
    assert pyutl.has_type("InputGeoPoint")

    cls = pyutl.parse_tl("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;", 177, pyutl.TLSection.TYPES)
    assert cls
    assert issubclass(cls, pyutl.TLObject)
    assert cls is not pyutl.TLObject

    assert pyutl.has_type("InputGeoPoint")
    assert not pyutl.has_type("InputGeoPointa")

    assert issubclass(pyutl.get_type("InputGeoPoint"), pyutl.TLType)
    assert pyutl.get_type("InputGeoPointa") is None
    assert pyutl.get_type("InputGeoPoint") is InputGeoPointBase

    assert pyutl.has_constructor(0x48222faf)
    assert not pyutl.has_constructor(0x48222fae)

    assert pyutl.get_constructor(0x48222faf) is cls


@pytest.mark.skipif(SKIP_TESTS >= 2, reason="")
def test_create_object() -> None:
    cls = pyutl.parse_tl("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;", 177, pyutl.TLSection.TYPES)
    assert cls

    obj = cls(lat=123.456, long=456.123)
    assert obj.lat == 123.456
    assert obj.long == 456.123
    assert obj.accuracy_radius is None

    obj.long = 123.789
    obj.accuracy_radius = 111
    assert obj.long == 123.789
    assert obj.accuracy_radius == 111


@pytest.mark.skipif(SKIP_TESTS >= 3, reason="")
def test_encode_object() -> None:
    cls = pyutl.parse_tl("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;", 177, pyutl.TLSection.TYPES)
    assert cls

    obj = cls(lat=42.24, long=24.42)
    assert obj.write() == bytes([0xaf, 0x2f, 0x22, 0x48, 0x0, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40])

    obj.accuracy_radius = 456123
    assert obj.write() == bytes([0xaf, 0x2f, 0x22, 0x48, 0x01, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40, 0xbb, 0xf5, 0x06, 0x00])


@pytest.mark.skipif(SKIP_TESTS >= 4, reason="")
def test_decode_object() -> None:
    cls = pyutl.parse_tl("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;", 177, pyutl.TLSection.TYPES)
    assert cls

    obj = cls.read_bytes(bytes([0x0, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40]))
    assert obj.lat == 42.24
    assert obj.long == 24.42
    assert obj.accuracy_radius is None

    obj = cls.read_bytes(bytes([0x01, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40, 0xbb, 0xf5, 0x06, 0x00]))
    assert obj.lat == 42.24
    assert obj.long == 24.42
    assert obj.accuracy_radius == 456123

    obj = pyutl.TLObject.read_bytes(bytes([0xaf, 0x2f, 0x22, 0x48, 0x0, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40]))
    assert obj.lat == 42.24
    assert obj.long == 24.42
    assert obj.accuracy_radius is None

    obj = pyutl.TLObject.read_bytes(bytes([0xaf, 0x2f, 0x22, 0x48, 0x01, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40, 0xbb, 0xf5, 0x06, 0x00]))
    assert obj.lat == 42.24
    assert obj.long == 24.42
    assert obj.accuracy_radius == 456123


@pytest.mark.skipif(SKIP_TESTS >= 5, reason="")
def test_nested_message() -> None:
    inputPeerUser = pyutl.parse_tl("inputPeerUser#dde8a54c user_id:long access_hash:long = InputPeer;", 177, pyutl.TLSection.TYPES)
    assert inputPeerUser

    inputUserFromMessage = pyutl.parse_tl("inputUserFromMessage#1da448e2 peer:InputPeer msg_id:int user_id:long = InputUser;", 177, pyutl.TLSection.TYPES)
    assert inputUserFromMessage

    obj = inputUserFromMessage(peer=inputPeerUser(user_id=123, access_hash=456), msg_id=789, user_id=123123)
    assert obj.msg_id == 789
    assert obj.user_id == 123123
    assert obj.peer.user_id == 123
    assert obj.peer.access_hash == 456

    serialized = b'\xe2H\xa4\x1dL\xa5\xe8\xdd{\x00\x00\x00\x00\x00\x00\x00\xc8\x01\x00\x00\x00\x00\x00\x00\x15\x03\x00\x00\xf3\xe0\x01\x00\x00\x00\x00\x00'
    assert obj.write() == serialized

    obj2 = inputUserFromMessage.read_bytes(serialized[4:])
    assert obj2.msg_id == obj.msg_id
    assert obj2.user_id == obj.user_id
    assert obj2.peer.user_id == obj.peer.user_id
    assert obj2.peer.access_hash == obj.peer.access_hash

    obj2 = pyutl.TLObject.read_bytes(serialized)
    assert obj2.msg_id == obj.msg_id
    assert obj2.user_id == obj.user_id
    assert obj2.peer.user_id == obj.peer.user_id
    assert obj2.peer.access_hash == obj.peer.access_hash


@pytest.mark.skipif(SKIP_TESTS >= 6, reason="")
def test_equals() -> None:
    inputPeerUser = pyutl.parse_tl("inputPeerUser#dde8a54c user_id:long access_hash:long = InputPeer;", 177, pyutl.TLSection.TYPES)
    assert inputPeerUser

    inputUserFromMessage = pyutl.parse_tl("inputUserFromMessage#1da448e2 peer:InputPeer msg_id:int user_id:long = InputUser;", 177, pyutl.TLSection.TYPES)
    assert inputUserFromMessage

    obj = inputUserFromMessage(peer=inputPeerUser(user_id=123, access_hash=456), msg_id=789, user_id=123123)
    assert obj.msg_id == 789
    assert obj.user_id == 123123
    assert obj.peer.user_id == 123
    assert obj.peer.access_hash == 456

    obj2 = pyutl.TLObject.read_bytes(obj.write())
    assert obj2.msg_id == obj.msg_id
    assert obj2.user_id == obj.user_id
    assert obj2.peer.user_id == obj.peer.user_id
    assert obj2.peer.access_hash == obj.peer.access_hash

    assert obj2 == obj
    assert obj2.peer == obj.peer
    assert obj2 != obj.peer
    assert obj2.peer != obj
    assert obj2 is not obj
    assert obj2.peer is not obj.peer
    assert obj2 is not obj.peer
    assert obj2.peer is not obj


@pytest.mark.skipif(SKIP_TESTS >= 7, reason="")
def test_vector() -> None:
    photoSizeProgressive = pyutl.parse_tl("photoSizeProgressive#fa3efb95 type:string w:int h:int sizes:Vector<int> = PhotoSize;", 177, pyutl.TLSection.TYPES)
    assert photoSizeProgressive

    obj = photoSizeProgressive(type="test", w=123, h=456, sizes=[1, 2, 3, 4, 5])
    assert obj.sizes[0] == 1
    assert obj.sizes[4] == 5
    obj.sizes.append(6)
    obj.sizes[2] = 9
    assert obj.sizes[5] == 6
    assert obj.sizes[2] == 9

    serialized = b'\x95\xfb>\xfa\x04test\x00\x00\x00{\x00\x00\x00\xc8\x01\x00\x00\x15\xc4\xb5\x1c\x06\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\t\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00'
    assert obj.write() == serialized

    bio = BytesIO(serialized[4:])
    obj2 = photoSizeProgressive.read(bio)
    assert obj2.type == obj.type
    assert obj2.w == obj.w
    assert obj2.h == obj.h
    assert obj2.sizes[0] == obj.sizes[0]
    assert obj2.sizes[4] == obj.sizes[4]
    assert obj2.sizes == obj.sizes
    assert obj2.sizes is not obj.sizes
    assert bio.tell() == len(bio.getvalue())
    assert bio.read() == b""


@pytest.mark.skipif(SKIP_TESTS >= 8, reason="")
def test_creation_from_python_type_annotated_class() -> None:
    idk = pyutl.parse_tl("idk#123456 qwe:string = IdkBase;", 177, pyutl.TLSection.TYPES)

    SomethingBase = pyutl.create_type("SomethingBase")
    IdkBase = pyutl.get_type("IdkBase")

    # TODO: maybe somehow make it inherit TLObject so .read and .write would be visible for pycharm and other tools
    class Something(pyutl.AnnotatedTLObject[SomethingBase]):
        __tl_id__ = 0x12345678
        __layer__ = 177
        __section__ = pyutl.TLSection.TYPES  # Optional, pyutl.TLSection.TYPES by default
        __tl__ = None  # TODO: Optional, may be tl definition to skip translation of python class to tl string

        some_int: pyutl.TLInt
        some_long: pyutl.TLLong
        some_int128: pyutl.TLInt128
        some_int256: pyutl.TLInt256
        some_double: float
        some_string: str
        some_bytes: bytes
        some_bool: bool
        some_object: IdkBase
        some_primitive_vector: list[pyutl.TLInt]
        some_object_vector: list[IdkBase]

        flags: pyutl.TLFlags
        some_optional_int: pyutl.TLOptional[pyutl.TLInt, 1]
        some_optional_long: pyutl.TLOptional[pyutl.TLLong, 2]
        some_optional_int128: pyutl.TLOptional[pyutl.TLInt128, 3]
        some_optional_int256: pyutl.TLOptional[pyutl.TLInt256, 4]
        some_optional_double: pyutl.TLOptional[float, 5]
        some_optional_string: pyutl.TLOptional[str, 6]
        some_optional_bytes: pyutl.TLOptional[bytes, 7]
        some_optional_object: pyutl.TLOptional[IdkBase, 8]
        some_optional_primitive_vector: pyutl.TLOptional[list[pyutl.TLInt], 9]
        some_optional_object_vector: pyutl.TLOptional[list[IdkBase], 10]
        some_optional_bool: pyutl.TLOptional[bool, 11]
        some_optional_true: pyutl.TLTrue[12]

        flags2: pyutl.TLFlags
        some_another_optional_int: pyutl.TLOptional[pyutl.TLInt, 1, 2]
        some_another_optional_long: pyutl.TLOptional[pyutl.TLLong, 2, 2]
        some_another_optional_int128: pyutl.TLOptional[pyutl.TLInt128, 3, 2]
        some_another_optional_int256: pyutl.TLOptional[pyutl.TLInt256, 4, 2]
        some_another_optional_double: pyutl.TLOptional[float, 5, 2]
        some_another_optional_string: pyutl.TLOptional[str, 6, 2]
        some_another_optional_bytes: pyutl.TLOptional[bytes, 7, 2]
        some_another_optional_object: pyutl.TLOptional[IdkBase, 8, 2]
        some_another_optional_primitive_vector: pyutl.TLOptional[list[pyutl.TLInt], 9, 2]
        some_another_optional_object_vector: pyutl.TLOptional[list[IdkBase], 10, 2]
        some_another_optional_bool: pyutl.TLOptional[bool, 11, 2]
        some_another_optional_true: pyutl.TLTrue[12, 2]

    assert issubclass(Something, pyutl.TLObject)

    # TODO: test Something.__tl__?
    # TODO: test serialization, deserialization, etc.
