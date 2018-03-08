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
				sources=['VSBIO/StandardLibrary/src/OFile.cpp',
						 'VSBIO/StandardLibrary/src/OSAbstraction.cpp',
						 'VSBIO/VSBIO/src/VSBIO.cpp',
						 'VSBIO/VSBIO/src/MessageTimeDecoderVSB.cpp',
						 'VSBIO/VSBIODLL.cpp',
						 'VSBIO/VSBIODLL.i'],
				swig_opts=['-c++', '-py3'],
				include_dirs=['VSBIO/VSBIO/include', 'VSBIO/StandardLibrary/include', 'VSBIO/Hardware/include', 'VSBIO/Core/include'],
				define_macros= define_macros)

setup(  name            = 'VSBIO',
		version         = '0.0.1',
		author          = 'Zaid Nackasha',
		author_email    = 'znackasha@intrepidcs.com',
		description     = '',
		url             = '',
		platforms       = ['x86_64'],
		packages        = ['VSBIO'],
		package_dir     = {'VSBIO': 'VSBIO'},
		cmdclass		= {'build': CustomBuild, 'install': CustomInstall},		
		ext_modules     = [module1],
		py_modules		= ['module1'] )
