#!/usr/bin/env python

import argparse
import os
from subprocess import call
import sys 

if sys.platform == 'win32':
    from msvcrt import getch
else:
    def getch():
        import tty, termios
        fd = sys.stdin.fileno()
        old = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            return sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("PROJECT_PATH", type=str, help="Path to the project")
    args = parser.parse_args()

    project_name = os.path.basename(args.PROJECT_PATH.strip('\\'))
    nap_root = os.path.abspath(os.path.join(args.PROJECT_PATH, os.pardir, os.pardir))
    script_path = os.path.join(nap_root, 'tools', 'packageProject.py')

    # Determine our Python interpreter location
    if sys.platform == 'win32':
        python = os.path.join(nap_root, 'thirdparty', 'python', 'python')
    else:
        python = 'python'

    call([python, script_path, project_name])

    # Pause to display output in case we're running from a file manager on Explorer / Finder
    # TODO Ideally work out if we're running from a terminal and don't ever pause if we are
    # TODO Discuss the possibility to only pause for input if we've hit an issue
    # TODO If we think it's a common use case that people are running this from a file manager in Linux do it for all
    if not sys.platform.startswith('linux'):
        print("Press key to close...")

        # Read a char from console
        getch()