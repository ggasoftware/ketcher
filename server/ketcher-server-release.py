import os
import shutil
import subprocess

if os.path.exists('build'):
    shutil.rmtree('build')
os.mkdir('build')
os.chdir('build')

# TODO: Multiarch
subprocess.check_call('cmake ..', shell=True)
subprocess.check_call('cmake --build .', shell=True)
subprocess.check_call('cmake --build . --target install', shell=True)

os.chdir('..')
if os.path.exists('libs'):
    shutil.rmtree('libs')
os.makedirs('libs')
shutil.copytree('build/install/shared', 'libs/shared')

shutil.rmtree('java/src/main/webapp/chem', ignore_errors=True)
shutil.rmtree('java/src/main/webapp/icons', ignore_errors=True)
shutil.rmtree('java/src/main/webapp/mol', ignore_errors=True)
shutil.rmtree('java/src/main/webapp/privat', ignore_errors=True)
shutil.rmtree('java/src/main/webapp/reaxys', ignore_errors=True)
shutil.rmtree('java/src/main/webapp/rnd', ignore_errors=True)
shutil.rmtree('java/src/main/webapp/ui', ignore_errors=True)
shutil.rmtree('java/src/main/webapp/util', ignore_errors=True)
shutil.copytree('../chem', 'java/src/main/webapp/chem')
shutil.copytree('../icons', 'java/src/main/webapp/icons')
shutil.copytree('../mol', 'java/src/main/webapp/mol')
shutil.copytree('../privat', 'java/src/main/webapp/privat')
shutil.copytree('../reaxys', 'java/src/main/webapp/reaxys')
shutil.copytree('../rnd', 'java/src/main/webapp/rnd')
shutil.copytree('../ui', 'java/src/main/webapp/ui')
shutil.copytree('../util', 'java/src/main/webapp/util')
shutil.copy('../base64.js', 'java/src/main/webapp/')
shutil.copy('../demo.html', 'java/src/main/webapp/')
shutil.copy('../favicon.ico', 'java/src/main/webapp/')
shutil.copy('../ketcher.css', 'java/src/main/webapp/')
shutil.copy('../ketcher.html', 'java/src/main/webapp/')
shutil.copy('../ketcher.js', 'java/src/main/webapp/')
shutil.copy('../loading.gif', 'java/src/main/webapp/')
shutil.copy('../prototype-min.js', 'java/src/main/webapp/')
shutil.copy('../raphael.js', 'java/src/main/webapp/')

os.chdir('java')
subprocess.check_call('mvn package', shell=True)
