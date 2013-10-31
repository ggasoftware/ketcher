import os
import shutil
import sys
import subprocess

from optparse import OptionParser

if os.path.exists('build'):
    shutil.rmtree('build')
os.mkdir('build')
os.chdir('build')

presets = {
    "win32" : ("Visual Studio 10", ""),
    "win64" : ("Visual Studio 10 Win64", ""),
    "win32-2012" : ("Visual Studio 11", ""),
    "win64-2012" : ("Visual Studio 11 Win64", ""),
    "win32-2013" : ("Visual Studio 12", ""),
    "win64-2013" : ("Visual Studio 12 Win64", ""),
    "win32-mingw": ("MinGW Makefiles", ""),
    "linux32" : ("Unix Makefiles", "-DSUBSYSTEM_NAME=x86"),
    "linux64" : ("Unix Makefiles", "-DSUBSYSTEM_NAME=x64"),
    "mac10.5" : ("Xcode", "-DSUBSYSTEM_NAME=10.5"),
    "mac10.6" : ("Xcode", "-DSUBSYSTEM_NAME=10.6"),
    "mac10.7" : ("Xcode", "-DSUBSYSTEM_NAME=10.7"),
    "mac10.8" : ("Xcode", "-DSUBSYSTEM_NAME=10.8"),
    "mac10.9" : ("Xcode", "-DSUBSYSTEM_NAME=10.9"),
}

parser = OptionParser(description='Indigo libraries build script')
parser.add_option('--generator', help='this option is passed as -G option for cmake')
parser.add_option('--preset', type="choice", dest="preset",
    choices=list(presets.keys()), help='build preset %s' % (str(presets.keys())))
parser.add_option('--config', default="Release", help='project configuration')

(args, left_args) = parser.parse_args()
if len(left_args) > 0:
    print("Unexpected arguments: %s" % (str(left_args)))
    exit()

if args.preset:
    args.generator, args.params = presets[args.preset]
if not args.generator:
    print("Generator must be specified")
    exit()                                        

if args.generator.find("Unix Makefiles") != -1 or args.generator.find("MinGW Makefiles") != -1:
    args.params += " -DCMAKE_BUILD_TYPE=" + args.config

command = 'cmake -G "{0}" ..'.format(args.generator)

subprocess.check_call(command, shell=True)
subprocess.check_call('cmake --build . --config %s' % (args.config), shell=True)
subprocess.check_call('cmake --build . --target install --config %s' % (args.config), shell=True)