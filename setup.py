from distutils.command.build import build
from setuptools.command.install import install
from distutils.core import setup, Extension
import distutils 
import platform

class CustomBuild(build):
	def run(self):
		self.run_command('build_ext')
		build.run(self)


class CustomInstall(install):
	def run(self):
		self.run_command('build_ext')
		self.do_egg_install()


if platform.system() == 'Windows':
	define_macros = [('_WIN32', None), ('VSBIODLL_EXPORTS', None)]
elif platform.system() == 'Linux': 
	define_macros = [('LINUXSO', None)]
define_macros += [('PYD', None)]

module1 = Extension('_VSBIOInterface',
				sources=['ICS_VSBIO/StandardLibrary/src/OFile.cpp',
						 'ICS_VSBIO/StandardLibrary/src/OSAbstraction.cpp',
						 'ICS_VSBIO/VSBIO/src/VSBIO.cpp',
						 'ICS_VSBIO/VSBIO/src/MessageTimeDecoderVSB.cpp',
						 'ICS_VSBIO/VSBIODLL.cpp',
						 'ICS_VSBIO/VSBIOInterface.i',
						 'ICS_VSBIO/VSBIOFlags.i'],
				swig_opts=['-c++', '-py3'],
				include_dirs=['ICS_VSBIO/VSBIO/include', 'ICS_VSBIO/StandardLibrary/include', 'ICS_VSBIO/Hardware/include', 'ICS_VSBIO/Core/include'],
				define_macros= define_macros)

module2 = Extension('_VSBIOFlags',
				sources=['ICS_VSBIO/VSBIOFlags.i'],
				swig_opts=['-c++', '-py3'],
				include_dirs=['ICS_VSBIO/VSBIO/include', 'ICS_VSBIO/StandardLibrary/include', 'ICS_VSBIO/Hardware/include', 'ICS_VSBIO/Core/include'],
				define_macros= define_macros)

setup(  name            = 'ICS_VSBIO',
		version         = '0.0.1',
		author          = 'Zaid Nackasha',
		author_email    = 'znackasha@intrepidcs.com',
		description     = '',
		url             = '',
		platforms       = ['x86_64'],
		packages        = ['ICS_VSBIO'],
		package_dir     = {'ICS_VSBIO': 'ICS_VSBIO'},
		cmdclass		= {'build': CustomBuild, 'install': CustomInstall},		
		ext_modules     = [module1, module2],
		py_modules		= ['module1', 'module2'] )
