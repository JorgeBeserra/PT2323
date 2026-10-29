"""Compile and run the real PT2323 source with a mock I2C bus.
Usage: python tests/run_tests.py --compiler path/to/zig.exe
GCC and Clang C++ drivers are also supported.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--compiler", required=True)
parser.add_argument("--source-dir", type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = args.source_dir.resolve() if args.source_dir else root / "src"
compiler = Path(args.compiler)
command = [str(compiler)]
if compiler.stem == "zig":
    command += ["c++"]
    if sys.platform == "win32":
        command += ["-target", "x86_64-windows-gnu"]
with tempfile.TemporaryDirectory(prefix="pt2323-tests-") as tmp:
    exe = Path(tmp) / "pt2323-tests.exe"
    command += ["-std=c++17", "-I" + str(root / "tests/mocks"), "-I" + str(source), str(root / "tests/test_commands.cpp"), str(source / "PT2323.cpp"), "-o", str(exe)]
    subprocess.run(command, check=True)
    subprocess.run([str(exe)], check=True)
