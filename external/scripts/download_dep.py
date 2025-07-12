import os
import subprocess
import tarfile
import urllib.request

def download_dep(depName, fileUrl, archiveFilename, extractDirName):
    if os.path.isdir(extractDirName):
        print("Skipping %s - already exists" % depName)
        return

    print("[Preparing %s]" % depName)

    if not os.path.isfile(archiveFilename):
        print("- %s archive doesn\'t exist, downloading it" % depName)

        # Download the dep
        urllib.request.urlretrieve(fileUrl, archiveFilename)

    # Extract the archive
    with tarfile.open(archiveFilename) as f:
        f.extractall()

    print("[Prepared %s]" % depName)
    
