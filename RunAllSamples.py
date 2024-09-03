# !/usr/bin/env python3

import os
import pathlib
import subprocess

current_dir = pathlib.Path(__file__).parent.resolve()
executable_dir = os.path.join(pathlib.Path(__file__).parent.resolve(), "build/bin")
print(executable_dir)
all_files = os.listdir(executable_dir)

for program_exec_path in all_files:
    print(program_exec_path)
    command = [program_exec_path, "--time", "10", "--fullscreen", "--vsync"]
    subprocess.run(command, shell=True, cwd=current_dir)
