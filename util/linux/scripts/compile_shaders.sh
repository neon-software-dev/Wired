#!/bin/sh


DEBUG_FLAGS=""

# Parse command-line arguments
while [ $# -gt 0 ]; do
  case "$1" in
    --debug)
      DEBUG_FLAGS="-g -O0"
      shift
      ;;
    --no-debug)
      DEBUG_FLAGS="-O"
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [--debug | --no-debug]"
      exit 1
      ;;
  esac
done

compile_shaders() {
  local tag=$1
  local shader_dir=$2

  echo "[Compiling shaders in $tag]"
  cd "$shader_dir" || exit 1

  for ext in vert frag comp; do
    for file in ./*.$ext; do
      [ -f "$file" ] || continue
      echo "Compiling $file..."
      glslc --target-env=vulkan1.3 $DEBUG_FLAGS "$file" -o "$file.spv"
    done
  done
}

# Compile default shaders
compile_shaders "default" "../../../src/default_shaders/"

# Compile WiredEditor shaders
compile_shaders "WiredEditor" "../WiredEditor/wired/packages/EditorPackage/assets/shaders/"

