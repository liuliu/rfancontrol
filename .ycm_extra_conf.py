import os
import ycm_core
from clang_helpers import PrepareClangFlags

def FlagsForFile(filename):
	return {
		'flags' : ['-Wall', '-I"/usr/local/cuda/include"],
		'do_cache' : True
	}
