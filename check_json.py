import tempfile, pathlib, os, json

d = tempfile.mkdtemp()
out = os.path.join(d, "test_sphere.urdf")

from urdf_approx_geom._extension import load_extension
_ext = load_extension()
msg = _ext.spherized("/workspace/resources/fr3/urdf/fr3.urdf", out, "/workspace/python/urdf_approx_geom/config/sphere.single.yml", [], True)
print("MSG:", msg)

json_path = out.replace(".urdf", ".json")
if os.path.exists(json_path):
    data = json.loads(open(json_path).read())
    print("JSON type:", type(data).__name__)
    if isinstance(data, dict):
        keys = list(data.keys())[:5]
        print("JSON keys:", keys)
        for k in keys[:2]:
            v = data[k]
            print(f"Key {k}: type={type(v).__name__}")
            if isinstance(v, dict):
                print(f"  dict keys: {list(v.keys())}")
            elif v is None:
                print(f"  value is None")
    elif isinstance(data, list):
        print("JSON is a list, length:", len(data))
else:
    print("No JSON found at", json_path)
