import os
print(f"\nPid: {os.getpid()}")
input("Press enter to continue...")

import _pyutl


def test_parse() -> None:
    pool = _pyutl.DefPool()
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


def test_create_object() -> None:
    pool = _pyutl.DefPool()
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


def test_encode_object() -> None:
    pool = _pyutl.DefPool()
    assert pool

    cls = pool.parse("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;")
    assert cls

    obj = cls(lat=42.24, long=24.42)
    assert obj.write() == bytes([0xaf, 0x2f, 0x22, 0x48, 0x0, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40])

    obj.accuracy_radius = 456123
    assert obj.write() == bytes([0xaf, 0x2f, 0x22, 0x48, 0x01, 0x0, 0x0, 0x0, 0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x45, 0x40, 0xec, 0x51, 0xb8, 0x1e, 0x85, 0x6b, 0x38, 0x40, 0xbb, 0xf5, 0x06, 0x00])


def test_decode_object() -> None:
    pool = _pyutl.DefPool()
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
