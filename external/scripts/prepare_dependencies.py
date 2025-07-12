import os
import argparse

from prepare_vcpkg import prepare_vcpkg
from download_dep import download_dep

parser = argparse.ArgumentParser(description='Provides Wired dependencies')

args = parser.parse_args()

print("[Preparing dependencies]")

external_dir = os.getcwd()

#######
# vcpkg
os.chdir(external_dir)
prepare_vcpkg(args)

######
# SDL3
#os.chdir(external_dir)
#download_dep('SDL3', 'https://github.com/libsdl-org/SDL/releases/download/release-3.2.10/SDL3-3.2.10.tar.gz', 'SDL3-3.2.10.tar.gz', 'SDL3-3.2.10')

######
# SDL3-image
#os.chdir(external_dir)
#download_dep('SDL3_image', 'https://github.com/libsdl-org/SDL_image/releases/download/release-3.2.4/SDL3_image-3.2.4.tar.gz', 'SDL3_image-3.2.4.tar.gz', 'SDL3_image-3.2.4')

