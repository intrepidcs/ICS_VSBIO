'''
Setup file for ICS_VSBIO
'''
from distutils.command.build import build
from setuptools.command.install import install
from distutils.core import setup, Extension
import platform

VERSION = '0.4.8'

def which(program):
    '''
    returns the location of the application you requested
    '''
    import os
    def is_exe(f_path):
        return os.path.isfile(f_path) and os.access(f_path, os.X_OK)

    f_path, _ = os.path.split(program)
    if f_path:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None

class CustomBuild(build):
    def run(self):
        self.run_command('build_ext')
        build.run(self)


class CustomInstall(install):
    def run(self):
        self.run_command('build_ext')
        self.do_egg_install()


if platform.system() == 'Windows':
    DEFINE_MACROS = [('_WIN32', None), ('VSBIODLL_EXPORTS', None)]
elif platform.system() == 'Linux':
    DEFINE_MACROS = [('LINUXSO', None)]
DEFINE_MACROS += [('PYD', None)]

if which('swig') or which('swig.exe'):
    SWIG_OPTS = ['-c++', '-py3']
    FLAGS_SOURCES = ['ICS_VSBIO/VSBIOFlags.i']
    INTERFACE_SOURCES = ['ICS_VSBIO/StandardLibrary/src/OFile.cpp',
                         'ICS_VSBIO/VSBIO/src/VSBIO.cpp',
                         'ICS_VSBIO/VSBIO/src/MessageTimeDecoderVSB.cpp',
                         'ICS_VSBIO/VSBIODLL.cpp',
                         'ICS_VSBIO/VSBIOInterface.i',
                         'ICS_VSBIO/VSBIOFlags.i']
else:
    SWIG_OPTS = None
    FLAGS_SOURCES = ['ICS_VSBIO/VSBIOFlags_wrap.cpp']
    INTERFACE_SOURCES = ['ICS_VSBIO/StandardLibrary/src/OFile.cpp',
                         'ICS_VSBIO/VSBIO/src/VSBIO.cpp',
                         'ICS_VSBIO/VSBIO/src/MessageTimeDecoderVSB.cpp',
                         'ICS_VSBIO/VSBIODLL.cpp',
                         'ICS_VSBIO/VSBIOInterface_wrap.cpp',
                         'ICS_VSBIO/VSBIOFlags_wrap.cpp']

if SWIG_OPTS is not None:
    for define, value in DEFINE_MACROS:
        SWIG_OPTS.append("-D" + define)

VSBIO_INTERFACE = Extension('_VSBIOInterface',
                            sources=INTERFACE_SOURCES,
                            swig_opts=SWIG_OPTS,
                            include_dirs=['ICS_VSBIO/VSBIO/include',
                                          'ICS_VSBIO/StandardLibrary/include',
                                          'ICS_VSBIO/Hardware/include',
                                          'ICS_VSBIO/Core/include'],
                            define_macros=DEFINE_MACROS)

VSBIO_FLAGS = Extension('_VSBIOFlags',
                        sources=FLAGS_SOURCES,
                        swig_opts=SWIG_OPTS,
                        include_dirs=['ICS_VSBIO/VSBIO/include',
                                      'ICS_VSBIO/StandardLibrary/include',
                                      'ICS_VSBIO/Hardware/include',
                                      'ICS_VSBIO/Core/include'],
                        define_macros=DEFINE_MACROS)

setup(name='ICS_VSBIO',
      version=VERSION,
      author='Zaid Nackasha',
      author_email='znackasha@intrepidcs.com',
      description='Python package to help users read and write Intrepid\'s vsb files',
      url='https://github.com/intrepidcs/ICS_VSBIO',
      download_url='https://github.com/intrepidcs/ICS_VSBIO/archive/' + VERSION + '.tar.gz',
      platforms=['x86_64'],
      packages=['ICS_VSBIO'],
      package_dir={'ICS_VSBIO': 'ICS_VSBIO'},
      cmdclass={'build': CustomBuild, 'install': CustomInstall},
      ext_modules=[VSBIO_INTERFACE, VSBIO_FLAGS],
      py_modules=['VSBIO_INTERFACE', 'VSBIO_FLAGS'])
