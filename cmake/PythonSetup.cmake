set(SETUP_PY_TEMPLATE
    "
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from wheel.bdist_wheel import bdist_wheel
from setuptools.command.sdist import sdist
import sys
import os
import subprocess
import multiprocessing
import shutil

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        try:
            subprocess.check_call(['cmake', '--version'])
        except OSError as e:
            raise RuntimeError('CMake must be installed to build the following extensions: ' +
                               ', '.join(e.name for e in self.extensions))

        super().run()

        # Only generate stubs if pybind11-stubgen is available
        try:
            subprocess.check_call(['pybind11-stubgen', '--help'],
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL)
            for ext in self.extensions:
                self.generate_stubs(ext)
        except (OSError, subprocess.CalledProcessError):
            print('Warning: pybind11-stubgen not found, skipping stub generation')

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = [
            '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
            '-DPYTHON_EXECUTABLE=' + self.get_python_executable(),
            '-DENABLE_PYBINDING=ON'
        ]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        num_cores = str(multiprocessing.cpu_count())
        build_args += ['--', '-j' + num_cores]

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

    def generate_stubs(self, ext):
        module_name = ext.name.split('.')[-1]
        stubs_dir = os.path.join(os.path.dirname(self.get_ext_fullpath(ext.name)), 'stubs')
        ext_dir = os.path.dirname(self.get_ext_fullpath(ext.name))

        os.makedirs(stubs_dir, exist_ok=True)

        try:
            # Add the extension directory to Python path so pybind11-stubgen can find the module
            env = os.environ.copy()
            if 'PYTHONPATH' in env:
                env['PYTHONPATH'] = f\"{ext_dir}:{env['PYTHONPATH']}\"
            else:
                env['PYTHONPATH'] = ext_dir

            subprocess.check_call([
                'pybind11-stubgen',
                module_name,
                '--output-dir', stubs_dir,
                '--ignore-invalid-expressions=all',
                '--ignore-invalid-identifiers=all',
            ], env=env)

            # Create stub package following PEP 561 for C extension with submodules
            # Use {module_name}-stubs/ directory structure
            generated_stub_dir = os.path.join(stubs_dir, module_name)
            target_dir = os.path.dirname(self.get_ext_fullpath(ext.name))
            stub_package_dir = os.path.join(target_dir, f'{module_name}-stubs')

            if os.path.exists(generated_stub_dir):
                # Create stub package directory
                os.makedirs(stub_package_dir, exist_ok=True)

                # Copy all stub files to stub package
                for item in os.listdir(generated_stub_dir):
                    if item.endswith('.pyi'):
                        item_path = os.path.join(generated_stub_dir, item)
                        target_stub = os.path.join(stub_package_dir, item)
                        shutil.copy2(item_path, target_stub)
                        print(f'Successfully generated stub: {target_stub}')

                # Create py.typed marker
                py_typed = os.path.join(stub_package_dir, 'py.typed')
                with open(py_typed, 'w') as f:
                    f.write('')
                print(f'Created py.typed marker: {py_typed}')

            shutil.rmtree(stubs_dir, ignore_errors=True)
        except subprocess.CalledProcessError as e:
            print(f'Warning: Failed to generate stubs for {module_name}: {e}')

    def get_python_executable(self):
        return os.path.abspath(sys.executable)

class WheelOnly(bdist_wheel):
    def run(self):
        # Disable sdist generation by overriding the sdist command
        self.distribution.cmdclass['sdist'] = lambda: None
        super().run()

setup(
    name='@PROJECT_NAME@_interface_py',
    version='@PROJECT_VERSION@',
    author='@AUTHOR@',
    author_email='@AUTHOR_EMAIL@',
    description='@PROJECT_DESCRIPTION@',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    url='https://irmv.sjtu.edu.cn/',
    packages=[],  # No Python packages, only C extension module with stubs
    ext_modules=[CMakeExtension('@PROJECT_NAME@_interface_py', sourcedir='.')],
    cmdclass={
        'build_ext': CMakeBuild,
        'bdist_wheel': WheelOnly,
    },
    zip_safe=False,
    install_requires=['pybind11-stubgen>=0.12.0', 'wheel>=0.36.0'],
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: Creative Commons Attribution-NonCommercial 4.0 International License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.8',
    package_data={
        '@PROJECT_NAME@_interface_py': ['*.pyi', 'py.typed'],
    },
    include_package_data=True,
    options={
        'bdist_wheel': {'universal': False},
        'sdist': {'formats': []},  # Disable sdist formats
    },
)
")

# Write the template to a file
file(WRITE ${CMAKE_BINARY_DIR}/setup.py.in "${SETUP_PY_TEMPLATE}")

# Configure the setup.py file
configure_file(${CMAKE_BINARY_DIR}/setup.py.in ${CMAKE_BINARY_DIR}/setup.py
               @ONLY)

# Add a custom target to generate setup.py
add_custom_target(
  generate_setup_py ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/setup.py
          ${CMAKE_SOURCE_DIR}/setup.py
  DEPENDS ${CMAKE_BINARY_DIR}/setup.py
  COMMENT "Generating setup.py")
