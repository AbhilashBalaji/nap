#!/usr/bin/env python3
import os
import subprocess
import sys

REQUIRED_UBUNTU_VERSION = '18.04'


def call(cmd):
    """Execute command and return stdout"""

    # print("Command: %s" % cmd)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    (out, _) = proc.communicate()
    return out.strip().decode('utf-8')

def log_test_success(test_item, success):
    """Log a test success"""

    print("Checking %s: %s" % (test_item, 'PASS' if success else 'FAIL'))

def check_arch():
    """Check if the we're running on a 64bit machine"""

    arch = call('uname -m')
    arch_ok = arch == 'x86_64'
    log_test_success('x86-64 architecture', arch_ok)
    return arch_ok

def check_distribution():
    """Check if the we're running Ubuntu"""

    distribution = call('lsb_release -id | grep ID')
    distributor_id = distribution.split(':')[1].strip().lower()
    distribution_ok = distributor_id == 'ubuntu'
    log_test_success('Ubuntu distribution', distribution_ok)
    return distribution_ok    

def check_distribution_version():
    """Check if the we're running our supported distribution version"""

    release = call('lsb_release -r')
    release = release.split(':')[1].strip()
    release_ok = release == REQUIRED_UBUNTU_VERSION
    log_test_success('Ubuntu %s' % REQUIRED_UBUNTU_VERSION, release_ok)
    return release_ok

def apt_package_installed(package_name):
    """Check if a package is installed via apt"""

    list_output = call('dpkg -l %s | grep %s' % (package_name, package_name))
    return list_output.startswith('ii')

def check_compiler():
    """Check that c++ is setup for GCC"""
    
    alternatives_output = call('update-alternatives --query c++ | grep Value')
    gcc_ok = alternatives_output == '' or '/g++' in alternatives_output

    log_test_success('C++ is GCC', gcc_ok)
    return gcc_ok 

def read_yes_no(question):
    """Read a yes/no answer for a question"""

    yes = ('yes','y', 'ye', '')
    no = ('no','n')
     
    while True:
        prompt = question + ' [Y/n] '
        if sys.version_info >= (3, 0):
            choice = input(prompt)
        else:
            choice = raw_input(prompt)
        choice = choice.lower().strip()
        if choice in yes:
           return True
        elif choice in no:
           return False
        else:
           print("Please respond with 'yes' or 'no'\n")

def check_build_environment():
    """Check whether Linux build environment appears ready for NAP"""

    # Check if 64bit
    arch_ok = check_arch()
    if not arch_ok:
        print("\nNAP only supports x86-64 systems. Not continuing checks.")
        sys.exit(1)

    # Check distribution
    distribution_ok = check_distribution()
    if not distribution_ok:
        print("\nThis version of NAP supports Ubuntu Linux (%s).  Other distributions may work but are unsupported." % REQUIRED_UBUNTU_VERSION)
        print("Hint for the adventurous: On Ubuntu we depend on build-essential, libglu1-mesa-dev and patchelf")
        print("\nNot continuing checks.")
        sys.exit(1)

    # Check distribution version
    distribution_version_ok = check_distribution_version()

    # Check package build-essential installed
    build_essential_installed = apt_package_installed('build-essential')
    log_test_success('for build-essential package', build_essential_installed)

    # Check C++ is GCC
    compiler_ok = check_compiler()

    # Check package patchelf installed
    patchelf_installed = apt_package_installed('patchelf')
    log_test_success('for patchelf package', patchelf_installed)

    # Check package libglu1-mesa-dev installed
    glut_intalled = apt_package_installed('libglu1-mesa-dev')
    log_test_success('for libglu1-mesa-dev package', glut_intalled)

    print("")

    # If everything looks good log and exit
    if distribution_version_ok and build_essential_installed and patchelf_installed and compiler_ok:
        print("Your build environment appears to be ready for NAP!")
        return False

    print("Some issues were encountered:")

    # Warn about wrong Ubuntu version
    if not distribution_version_ok:
        print("\nWarning: This version of NAP is supported on Ubuntu %s.  Other Linux configurations may work but are unsupported." % REQUIRED_UBUNTU_VERSION)

    if not compiler_ok:
        print("\nYour C++ compiler is not currently set to GCC. This release of NAP only currently supports GCC.")
        return False

    # Build apt packages to offer to install
    packages_to_install = []
    if not build_essential_installed:
        packages_to_install.append('build-essential')
    if not patchelf_installed:
        packages_to_install.append('patchelf')
    if not glut_intalled:
        packages_to_install.append('libglu1-mesa-dev')
        
    if len(packages_to_install) > 0:
        package_str = ' '.join(packages_to_install)
        print("\nThe following package/s are required and are not installed: %s" % package_str)
        installation_approved = read_yes_no("Kick off installation via apt?")
        if installation_approved:
            subprocess.call('sudo apt-get install %s' % package_str, shell=True)
            return True
        else:
            print("Re-run checkBuildEnvironment once you have installed the requirements.")
            return False

if __name__ == '__main__':
     while check_build_environment():
         print("\nRe-running checks...\n")
