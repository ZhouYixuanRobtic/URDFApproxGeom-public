import tempfile, pathlib, os, json, sys
sys.path.insert(0, "/workspace/python")
sys.path.insert(0, "/workspace/build/python")

d = tempfile.mkdtemp()
out = os.path.join(d, "test_sphere.urdf")

from urdf_approx_geom.api import generate
result = generate("sphere", "/workspace/resources/fr3/urdf/fr3.urdf", out, preset="default")
print("Mode:", result.mode)
print("Message:", result.message)

if result.json_path and os.path.exists(str(result.json_path)):
    data = json.loads(open(str(result.json_path)).read())
    print("JSON type:", type(data).__name__)
    if isinstance(data, dict):
        keys = list(data.keys())
        print("JSON keys count:", len(keys))
        print("First 3 keys:", keys[:3])
        for k in keys[:3]:
            v = data[k]
            print(f"  Key {k}: type={type(v).__name__}")
            if isinstance(v, dict):
                print(f"    dict keys: {list(v.keys())}")
            elif v is None:
                print(f"    value is None")
    elif isinstance(data, list):
        print("JSON is a list, length:", len(data))
        if data:
            print("First element:", type(data[0]).__name__)
            if isinstance(data[0], dict):
                print("First element keys:", list(data[0].keys()))
else:
    print("No JSON found")
