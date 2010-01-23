#!/usr/bin/env python

import subprocess

import Utils

APPNAME = 'maki'
VERSION = '1.2.0'

srcdir = '.'
blddir = 'build'

def _getconf (key):
	# FIXME Maybe use os.confstr().
	p = subprocess.Popen(['getconf', key], stdout=subprocess.PIPE, close_fds=True)
	v = p.communicate()[0].strip()

	if not v:
		return []

	return Utils.to_list(v)

def configure (conf):
	conf.env.CCFLAGS = ['-g', '-O', '-pipe', '-std=c99', '-Wall', '-pedantic']
	conf.env.LDFLAGS = []

	conf.env.CCFLAGS += ['-Wextra', '-Wno-missing-field-initializers', '-Wno-unused-parameter', '-Wold-style-definition', '-Wdeclaration-after-statement', '-Wmissing-declarations', '-Wmissing-prototypes', '-Wredundant-decls', '-Wmissing-noreturn', '-Wshadow', '-Wpointer-arith', '-Wcast-align', '-Wwrite-strings', '-Winline', '-Wformat-nonliteral', '-Wformat-security', '-Wswitch-enum', '-Wswitch-default', '-Winit-self', '-Wmissing-include-dirs', '-Wundef', '-Waggregate-return', '-Wmissing-format-attribute', '-Wnested-externs', '-Wstrict-prototypes']

	conf.env.CCFLAGS += _getconf('LFS_CFLAGS')
	conf.env.LDFLAGS += _getconf('LFS_LDFLAGS')
	conf.env.LDFLAGS += _getconf('LFS_LIBS')

	conf.env.VERSION = VERSION

	conf.sub_config('data')
	conf.sub_config('documentation')
	conf.sub_config('po')
	conf.sub_config('source')

def build (bld):
	bld.add_subdirs('data')
	bld.add_subdirs('documentation')
	bld.add_subdirs('po')
	bld.add_subdirs('source')
