#!/usr/bin/env python

APPNAME = 'maki'
VERSION = '1.4.0'

top = '.'
out = 'build'

def options (ctx):
	ctx.load('compiler_c')

def configure (ctx):
	ctx.load('compiler_c')
	ctx.load('gnu_dirs')

	ctx.env.CFLAGS += ['-std=c99', '-pedantic', '-Wall', '-Wextra']
	ctx.env.CFLAGS += ['-Wno-missing-field-initializers', '-Wno-unused-parameter', '-Wold-style-definition', '-Wdeclaration-after-statement', '-Wmissing-declarations', '-Wmissing-prototypes', '-Wredundant-decls', '-Wmissing-noreturn', '-Wshadow', '-Wpointer-arith', '-Wcast-align', '-Wwrite-strings', '-Winline', '-Wformat-nonliteral', '-Wformat-security', '-Wswitch-enum', '-Wswitch-default', '-Winit-self', '-Wmissing-include-dirs', '-Wundef', '-Waggregate-return', '-Wmissing-format-attribute', '-Wnested-externs', '-Wstrict-prototypes']

	ctx.env.CFLAGS += ['-D_FILE_OFFSET_BITS=64']

	ctx.env.VERSION = VERSION

	ctx.recurse('data')
	ctx.recurse('po')
	ctx.recurse('source')

def build (ctx):
	ctx.recurse('data')
	ctx.recurse('documentation')
	ctx.recurse('po')
	ctx.recurse('source')
