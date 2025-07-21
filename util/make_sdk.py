import platform
import os
import subprocess
import shutil
import sys
from pathlib import Path

def run_command(command, shell=False):
  print(f"Running: {' '.join(command) if isinstance(command, list) else command}")
  subprocess.run(command, shell=shell, check=True)

def main():
  script_dir = Path(__file__).resolve()
  wired_dir = script_dir.parents[1]
  external_dir = wired_dir / 'external'
  sdk_dir = wired_dir / 'sdk'
  sdk_build_dir = wired_dir / 'sdk_build'
    
  ####
  # Prepare dependencies
  os.chdir(external_dir)
  run_command([sys.executable, 'prepare_dependencies.py'])

  ####
  # Build and install Wired
  sdk_dir.mkdir(exist_ok=True)
  sdk_build_dir.mkdir(exist_ok=True)

  os.chdir(sdk_build_dir)

  # Use the distro preset for the current OS
  preset = 'desktop-distro-windows' if platform.system() == 'Windows' else 'desktop-distro-linux'

  # Invoke CMake
  run_command([
      'cmake', str(wired_dir / 'src'), '--preset', preset, '-DBUILD_SHARED_LIBS=ON', '-DWIRED_OPT_IMGUI=OFF', f'-DCMAKE_INSTALL_PREFIX={sdk_dir}'
  ])

  # Build multi-threaded
  if platform.system() == 'Windows':
    opt_multi = '/m'
  else:
    opt_multi = '-j'
    
  # Build
  run_command(['cmake', '--build', '.', '--config', 'Release', '--', opt_multi])
  run_command(['cmake', '--install', '.', '--config', 'Release', '--component', 'WiredEngine_Runtime'])
  run_command(['cmake', '--install', '.', '--config', 'Release', '--component', 'WiredEngine_Development'])

  #run_command(['cmake', '--build', '.', '--config', 'Debug', '--', opt_multi])
  #run_command(['cmake', '--install', '.', '--config', 'Debug', '--component', 'WiredEngine_Runtime'])
  #run_command(['cmake', '--install', '.', '--config', 'Debug', '--component', 'WiredEngine_Development'])

  ####
  # Copy transitive runtime dependencies into the SDK
  if platform.system() == 'Windows':
    deps_source_dir = external_dir / 'vcpkg' / 'installed' / 'x64-windows' / 'bin'
    deps_target_dir = sdk_dir / 'bin'
    deps_file_pattern = '*.dll'
  else:
    deps_source_dir = external_dir / 'vcpkg' / 'installed' / 'x64-linux-dynamic' / 'lib'
    deps_target_dir = sdk_dir / 'lib'
    deps_file_pattern = '*.so*'

  deps_target_dir.mkdir(exist_ok=True)
  for file in deps_source_dir.glob(deps_file_pattern):
      shutil.copy2(file, deps_target_dir)
        
  ####
  # Copy default shaders into the SDK
  (sdk_dir / "wired" / "default_shaders").mkdir(parents=True, exist_ok=True)
  for f in (wired_dir / "src" / "default_shaders").glob("*.spv"):
    shutil.copy(f, sdk_dir / "wired" / "default_shaders")

if __name__ == "__main__":
  main()

