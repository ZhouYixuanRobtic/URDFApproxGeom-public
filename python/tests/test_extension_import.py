import importlib


def test_public_package_imports_private_extension():
    pkg = importlib.import_module("urdf_approx_geom")
    assert hasattr(pkg, "capsuleized")
    assert hasattr(pkg, "convex")
    assert hasattr(pkg, "spherized")


def test_private_extension_is_not_the_public_package():
    pkg = importlib.import_module("urdf_approx_geom")
    ext = importlib.import_module("_urdf_approx_geom")
    assert pkg is not ext
    assert hasattr(ext, "capsuleized")
