

import _pyutl


def test_parse() -> None:
    pool = _pyutl.DefPool()
    assert pool

    addr = pool.parse("inputGeoPoint#48222faf flags:# lat:double long:double accuracy_radius:flags.0?int = InputGeoPoint;")
    assert addr

    assert pool.has_type("InputGeoPoint")
    assert not pool.has_type("InputGeoPointa")

    assert pool.has_constructor(0x48222faf)
    assert not pool.has_constructor(0x48222fae)

    assert pool.get_constructor(0x48222faf) == addr
