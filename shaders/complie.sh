#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------------------------
# Compile all GLSL shaders in this directory to SPIR-V.
# Usage: ./compile.sh   (run from inside the shaders/ folder)
# ---------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR"
BIN_DIR="$SCRIPT_DIR/bin"

# Pick a compiler: prefer glslc (bundled with the Vulkan SDK / shaderc),
# fall back to glslangValidator if that's what's on PATH.
if command -v glslc >/dev/null 2>&1; then
    COMPILER="glslc"
elif command -v glslangValidator >/dev/null 2>&1; then
    COMPILER="glslangValidator"
else
    echo "error: neither glslc nor glslangValidator found on PATH" >&2
    echo "       install the Vulkan SDK or add it to PATH" >&2
    exit 1
fi

mkdir -p "$BIN_DIR"

# Extensions we treat as shader stages.
SHADER_EXTS=(vert frag comp geom tesc tese rgen rchit rmiss)

shopt -s nullglob
found_any=0

for ext in "${SHADER_EXTS[@]}"; do
    for src in "$SRC_DIR"/*."$ext"; do
        found_any=1
        name="$(basename "$src")"
        out="$BIN_DIR/${name}.spv"

        echo "compiling: $name -> bin/${name}.spv"

        if [ "$COMPILER" = "glslc" ]; then
            glslc "$src" -o "$out"
        else
            # glslangValidator needs -S <stage> only for nonstandard
            # extensions; standard ones (.vert/.frag/etc.) are inferred.
            glslangValidator -V "$src" -o "$out"
        fi
    done
done

if [ "$found_any" -eq 0 ]; then
    echo "no shader files found in $SRC_DIR (looked for: ${SHADER_EXTS[*]})"
    exit 1
fi

echo "done."