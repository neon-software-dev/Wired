import os
import shutil
import subprocess


VCPKG_BASELINE_VERSION = "2025.06.13"

def ensure_vcpkg():
    if os.path.isdir('vcpkg'):
        print("vcpkg already exists, skipping")
        return

    subprocess.run(['git', 'clone', "https://github.com/microsoft/vcpkg.git"])
    os.chdir('vcpkg')
    
    subprocess.run(['git', 'checkout', VCPKG_BASELINE_VERSION])

    if os.name == 'nt':
        os.system('bootstrap-vcpkg.bat -disableMetrics')
    else:
        os.system('./bootstrap-vcpkg.sh --disableMetrics')

    print("[Prepared vcpkg]")
    

def install_dep(dep, dynamic):
    depStr = dep
    
    if os.name != 'nt' and dynamic:
      depStr = dep + ':x64-linux-dynamic'
    elif os.name != 'nt' and not dynamic:
      depStr = dep + ':x64-linux'
    
    # If the dependency should be dynamic, use the custom triplet
    #if os.name != 'nt' and dynamic:
    #	depStr = dep + ':x64-linux-dynamic'

    print('[Installing: %s]' % (depStr))
    
    if os.name == 'nt':
        os.system('vcpkg install %s' % (depStr))
    else:
        os.system('./vcpkg install %s' % (depStr))


def install_dependencies():
    # Switch to vcpkg dir
    os.chdir('vcpkg')
	
    #Install dependencies
    sdl3Dep = 'sdl3'
    if os.name == 'nt':
        sdl3Dep = 'sdl3[vulkan]'
    else:
        sdl3Dep = 'sdl3[vulkan,wayland,x11]'
    install_dep(sdl3Dep, False)
    
    install_dep('sdl3-ttf', False)
    install_dep('sdl3-image[jpeg,png]', False)
    install_dep('spirv-reflect', False)
    install_dep('glm', False)
    install_dep('entt', False)
    install_dep('imgui[sdl3-binding,vulkan-binding,docking-experimental]', False)
    install_dep('implot', False)
    install_dep('gtest', False)
    install_dep('nlohmann-json', False)
    install_dep('assimp', False)
    install_dep('vulkan-headers', False)
    install_dep('vulkan-memory-allocator', False)
    install_dep('joltphysics', False)
    install_dep('audiofile', False)
    install_dep('openal-soft', True)

    print("[Prepared vcpkg]")


def prepare_vcpkg(args):
    external_dir = os.getcwd()

    print("[Ensuring vcpkg]")
    ensure_vcpkg()
    
    print("[Installing dependencies]")
    os.chdir(external_dir)
    install_dependencies()
    

