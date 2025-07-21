import os
import argparse
import sys

subdir_path = os.path.join(os.path.dirname(__file__), 'internal')
sys.path.append(subdir_path)

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

