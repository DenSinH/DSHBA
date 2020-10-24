import os
import re
import sys

from warnings import warn
from pprint import pprint

try:
    import moderngl

    ctx = moderngl.create_standalone_context(require=430)
    print(f"Loaded OpenGL version {ctx.version_code}")

    def check_compile(text: str, name: str):
        try:
            ctx.compute_shader(text)
            print(f"Compiled shader '{name}'")
        except Exception as e:
            for i, line in enumerate(text.split("\n")):
                print(f"{i + 1:03d}: {line}")
            warn(f"Failed compiling shader '{name}': {e}")

except ImportError:
    print("Could not find module 'OpenGL.GL.shaders', skipping shader compilation check")

    moderngl = None
    ctx = None

    def check_compile(text: str, name: str):
        pass

SCRIPT, *_ = sys.argv
SHADER_DIR = f"{os.path.dirname(SCRIPT)}/raw"
CONSTANTS = f"{os.path.dirname(SCRIPT)}/GX_constants.h"
SHADER_H = "#ifndef GC__SHADER_H\n" \
           "#define GC__SHADER_H"

with open(CONSTANTS, "r") as f:
    constants = {}
    for line in f.readlines():
        match = re.match(r"^.*?(\w+).*=.*?(\w+).*$", line)
        if not match:

            match = re.match(r"^.*?#define (\w+)\s+(\w+).*$", line)
            if not match:
                continue
        constants[match.group(1)] = match.group(2)

print("Found the following constants:")
pprint(constants)

for file in os.listdir(SHADER_DIR):
    with open(os.path.join(SHADER_DIR, file), "r") as f:
        content = f.read().split("\n")

    i = 0
    while i < len(content):
        match = re.match(".*BEGIN\\s+(\w+)", content[i])
        i += 1
        if not match:
            continue

        shader_name = match.group(1)
        shader = [f"const char* {shader_name} = "]
        raw = ""

        # skip leading empty lines
        while not content[i]:
            i += 1

        shader_start = i

        while i < len(content) and not re.match(f".*END\\s+{shader_name}", content[i]):
            line = content[i]
            for const, value in constants.items():
                line = re.sub(f"\\+\\+{const}\\+\\+", value, line)

            raw += line + "\n"

            # escape backslashes and quotes
            line = line.replace("\\", "\\\\").replace("\"", "\\\"")

            shader.append(f'"{line}\\n"  // l:{i - shader_start + 1}')
            i += 1

        if i == len(content):
            raise OSError(f"EOF reached while scanning shader '{shader_name}'")

        # remove trailing newlines (only whitespace and quotes in text)
        while re.match(r"^(\s|(\\n)|\")*$", shader[-1]):
            shader.pop(-1)

        text = "\n".join(shader)

        # skip declaration on compile checking
        check_compile(raw, shader_name)

        # skip line with END in it
        i += 1
        SHADER_H += f"\n\n// {shader_name} (from {file}, lines {shader_start} to {i - 1})\n{text}\n;\n"

SHADER_H += "\n#endif  // GC__SHADER_H"

print("outputting shaders.h in ", f"{os.path.dirname(SCRIPT)}/shaders.h")

with open(f"{os.path.dirname(SCRIPT)}/shaders.h", "w+") as f:
    f.write(SHADER_H)
