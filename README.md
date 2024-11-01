# SpherizedURDFGenerator
An automatic tool for generating spherized / convex version of collision gemotry and write URDF automatically. 

# Introduction





# Dependencies

This package relies on binary-distributed libraries:

- [cgal](https://github.com/CGAL/cgal?tab=License-1-ov-file)
- [urdfdom](https://github.com/ros/urdfdom)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
- [tinyxml2](https://github.com/leethomason/tinyxml2)

Fortunately, they can be installed through simple `apt-get` process.  

```shell
sudo apt-get install libcgal-dev liburdfdom-dev libyaml-cpp-dev libtinyxml2-dev libgmp-dev
```

The distribution also contains the following sources from other people (some are introduced in a header-only manner):

- [libigl](https://github.com/libigl/libigl)
- [plog](https://github.com/SergiusTheBest/plog)
- [sphere_tree](https://github.com/mlund/spheretree)
- [ManifoldPlus](https://github.com/hjwdzh/ManifoldPlus)
- [cmake-template](https://github.com/cpp-best-practices/cmake_template/tree/main)

This work is built on the basis of those fantastic works. We thank all the aforementioned open-source project for the help of the implementations.

# Compile

```shell
cmake -B build . -DCMAKE_BUILD_TYPE=Release
cmake --build build 
```

# Usage



## Xacro

```shell
python -m xacro your_file.urdf.xacro -o output_file.urdf
```

# Example
