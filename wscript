#!/usr/bin/env python

import subprocess

from waflib import Utils

APPNAME = 'maki'
VERSION = '1.4.0'

top = '.'
out = 'build'

def _getconf (key):
	# FIXME Maybe use os.confstr().
	p = subprocess.Popen(['getconf', key], stdout=subprocess.PIPE, close_fds=True)
	v = p.communicate()[0].strip()

	if not v:
		return []

	return Utils.to_list(v)

def options (ctx):
	ctx.load('compiler_c')

def configure (ctx):
	ctx.load('c_config')
	ctx.load('compiler_c')
	ctx.load('glib2')
	ctx.load('gnu_dirs')

	ctx.env.CFLAGS += ['-std=c99', '-pedantic', '-Wall', '-Wextra', '-Wno-missing-field-initializers', '-Wno-unused-parameter', '-Wold-style-definition', '-Wdeclaration-after-statement', '-Wmissing-declarations', '-Wmissing-prototypes', '-Wredundant-decls', '-Wmissing-noreturn', '-Wshadow', '-Wpointer-arith', '-Wcast-align', '-Wwrite-strings', '-Winline', '-Wformat-nonliteral', '-Wformat-security', '-Wswitch-enum', '-Wswitch-default', '-Winit-self', '-Wmissing-include-dirs', '-Wundef', '-Waggregate-return', '-Wmissing-format-attribute', '-Wnested-externs', '-Wstrict-prototypes']

	ctx.env.CFLAGS += _getconf('LFS_CFLAGS')
	ctx.env.LDFLAGS += _getconf('LFS_LDFLAGS')
	ctx.env.LDFLAGS += _getconf('LFS_LIBS')

	ctx.env.VERSION = VERSION

	ctx.recurse('data')
	ctx.recurse('documentation')
	ctx.recurse('po')
	ctx.recurse('source')

def build (ctx):
	ctx.recurse('data')
	ctx.recurse('documentation')
	ctx.recurse('po')
	ctx.recurse('source')
