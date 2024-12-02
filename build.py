import _imp
import os
import re
import subprocess
import sys
from pathlib import Path

from setuptools import Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = "") -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        # Must be in this form due to bug in .resolve() only fixed in Python 3.10+
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()

        debug = int(os.environ.get("DEBUG", 0)) if self.debug is None else self.debug
        cfg = "Debug" if debug else "Release"

        cmake_generator = os.environ.get("CMAKE_GENERATOR", "")

        cmake_args = [
            f"-DPython3_EXECUTABLE={sys.executable}",
            f"-DPython3_ROOT_DIR={str(Path(sys.executable).parent.parent)}",
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}/pyutl",
            f"-DCMAKE_BUILD_TYPE={cfg}",
        ]
        build_args = [
            "--target=pyutl"
        ]

        if "CMAKE_ARGS" in os.environ:
            cmake_args.extend([item for item in os.environ["CMAKE_ARGS"].split(" ") if item])

        if self.compiler.compiler_type != "msvc" and (not cmake_generator or cmake_generator == "Ninja"):
            try:
                import ninja

                ninja_executable_path = Path(ninja.BIN_DIR) / "ninja"
                cmake_args.append("-GNinja")
                cmake_args.append(f"-DCMAKE_MAKE_PROGRAM:FILEPATH={ninja_executable_path}")
            except ImportError:
                pass

        else:
            # Single config generators are handled "normally"
            single_config = any(x in cmake_generator for x in {"NMake", "Ninja"})

            # Multi-config generators have a different way to specify configs
            if not single_config:
                cmake_args.append(f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}")
                build_args.append("--config")
                build_args.append(cfg)

        if sys.platform.startswith("darwin"):
            # Cross-compile support for macOS - respect ARCHFLAGS if set
            archs = re.findall(r"-arch (\S+)", os.environ.get("ARCHFLAGS", ""))
            if archs:
                cmake_args.append(f"-DCMAKE_OSX_ARCHITECTURES={';'.join(archs)}")

        build_args.append(f"-j6")

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)

        subprocess.run(
            ["cmake", ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=build_temp, check=True
        )


def build(setup_kwargs: dict):
    setup_kwargs.update({
        "ext_modules": [CMakeExtension(".")],
        "cmdclass": {
            "build_ext": CMakeBuild,
        },
    })
