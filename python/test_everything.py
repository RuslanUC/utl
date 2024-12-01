import os
from io import BytesIO

import pytest

print(f"\nPid: {os.getpid()}")
input("Press enter to continue...")

import _pyutl


SKIP_TESTS = 0


@pytest.mark.skipif(SKIP_TESTS >= 1, reason="")
def test_parse() -> None:
    pool = _pyutl.def_pool
    assert pool

    cls = pool.parse("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;")
    assert cls
    assert issubclass(cls, _pyutl.TLObject)
    assert cls is not _pyutl.TLObject

    assert pool.has_type("InputGeoPoint")
    assert not pool.has_type("InputGeoPointa")

    assert pool.has_constructor(0x48222faf)
    assert not pool.has_constructor(0x48222fae)

    assert pool.get_constructor(0x48222faf) is cls


@pytest.mark.skipif(SKIP_TESTS >= 2, reason="")
def test_create_object() -> None:
    pool = _pyutl.def_pool
    assert pool

    cls = pool.parse("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;")
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
    pool = _pyutl.def_pool
    assert pool

    cls = pool.parse("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;")
    assert cls

    obj = cls(lat=42.24, long=24.42)
    assert obj.write() == bytes([0xaf, 0x2f, 0x22, 0x48, 0x0, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40])

    obj.accuracy_radius = 456123
    assert obj.write() == bytes([0xaf, 0x2f, 0x22, 0x48, 0x01, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40, 0xbb, 0xf5, 0x06, 0x00])


@pytest.mark.skipif(SKIP_TESTS >= 4, reason="")
def test_decode_object() -> None:
    pool = _pyutl.def_pool
    assert pool

    cls = pool.parse("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;")
    assert cls

    obj = cls.read_bytes(bytes([0x0, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40]))
    assert obj.lat == 42.24
    assert obj.long == 24.42
    assert obj.accuracy_radius is None

    obj = cls.read_bytes(bytes([0x01, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40, 0xbb, 0xf5, 0x06, 0x00]))
    assert obj.lat == 42.24
    assert obj.long == 24.42
    assert obj.accuracy_radius == 456123

    obj = _pyutl.TLObject.read_bytes(bytes([0xaf, 0x2f, 0x22, 0x48, 0x0, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40]))
    assert obj.lat == 42.24
    assert obj.long == 24.42
    assert obj.accuracy_radius is None

    obj = _pyutl.TLObject.read_bytes(bytes([0xaf, 0x2f, 0x22, 0x48, 0x01, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40, 0xbb, 0xf5, 0x06, 0x00]))
    assert obj.lat == 42.24
    assert obj.long == 24.42
    assert obj.accuracy_radius == 456123


@pytest.mark.skipif(SKIP_TESTS >= 5, reason="")
def test_nested_message() -> None:
    pool = _pyutl.def_pool
    assert pool

    inputPeerUser = pool.parse("inputPeerUser#dde8a54c user_id:long access_hash:long = InputPeer;")
    assert inputPeerUser

    inputUserFromMessage = pool.parse("inputUserFromMessage#1da448e2 peer:InputPeer msg_id:int user_id:long = InputUser;")
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

    obj2 = _pyutl.TLObject.read_bytes(serialized)
    assert obj2.msg_id == obj.msg_id
    assert obj2.user_id == obj.user_id
    assert obj2.peer.user_id == obj.peer.user_id
    assert obj2.peer.access_hash == obj.peer.access_hash


@pytest.mark.skipif(SKIP_TESTS >= 6, reason="")
def test_equals() -> None:
    pool = _pyutl.def_pool
    assert pool

    inputPeerUser = pool.parse("inputPeerUser#dde8a54c user_id:long access_hash:long = InputPeer;")
    assert inputPeerUser

    inputUserFromMessage = pool.parse("inputUserFromMessage#1da448e2 peer:InputPeer msg_id:int user_id:long = InputUser;")
    assert inputUserFromMessage

    obj = inputUserFromMessage(peer=inputPeerUser(user_id=123, access_hash=456), msg_id=789, user_id=123123)
    assert obj.msg_id == 789
    assert obj.user_id == 123123
    assert obj.peer.user_id == 123
    assert obj.peer.access_hash == 456

    obj2 = _pyutl.TLObject.read_bytes(obj.write())
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
    pool = _pyutl.def_pool
    assert pool

    photoSizeProgressive = pool.parse("photoSizeProgressive#fa3efb95 type:string w:int h:int sizes:Vector<int> = PhotoSize;")
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

