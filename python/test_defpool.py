import os
print(os.getpid())
input()

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
