#!/usr/bin/env python

from waflib import Utils

APPNAME = 'maki'
VERSION = '1.4.0'

top = '.'
out = 'build'

def options (ctx):
	ctx.load('compiler_c')

	ctx.add_option('--debug', action='store_true', default=False, help='Enable debug mode')

def configure (ctx):
	ctx.load('compiler_c')
	ctx.load('gnu_dirs')
	ctx.load('intltool')

	ctx.find_program('gzip', var = 'GZIP')

	ctx.env.CFLAGS += ['-std=c99']
	ctx.env.CFLAGS += ['-D_FILE_OFFSET_BITS=64']

	#ctx.check_large_file()

	ctx.check_cfg(
		package = 'gio-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.26',
		uselib_store = 'GIO'
	)

	ctx.check_cfg(
		package = 'glib-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.26',
		uselib_store = 'GLIB'
	)

	ctx.check_cfg(
		package = 'gmodule-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.26',
		uselib_store = 'GMODULE'
	)

	ctx.check_cfg(
		package = 'gobject-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.26',
		uselib_store = 'GOBJECT'
	)

	ctx.check_cfg(
		package = 'gthread-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.26',
		uselib_store = 'GTHREAD'
	)

	ctx.check_cfg(
		package = 'nice',
		args = ['--cflags', '--libs'],
		mandatory = False
	)

	ctx.check_cfg(
		package = 'gupnp-1.0',
		args = ['--cflags', '--libs'],
		uselib_store = 'GUPNP',
		mandatory = False
	)

	ctx.check_cfg(
		package = 'gupnp-igd-1.0',
		args = ['--cflags', '--libs'],
		uselib_store = 'GUPNP_IGD',
		mandatory = False
	)

	ctx.check_cfg(
		package = 'dbus-1',
		variables = ['session_bus_services_dir'],
		uselib_store = 'DBUS',
		mandatory = False
	)

	if ctx.options.debug:
		ctx.env.CFLAGS += ['-pedantic', '-Wall', '-Wextra']
		ctx.env.CFLAGS += ['-Wno-missing-field-initializers', '-Wno-unused-parameter', '-Wold-style-definition', '-Wdeclaration-after-statement', '-Wmissing-declarations', '-Wmissing-prototypes', '-Wredundant-decls', '-Wmissing-noreturn', '-Wshadow', '-Wpointer-arith', '-Wcast-align', '-Wwrite-strings', '-Winline', '-Wformat-nonliteral', '-Wformat-security', '-Wswitch-enum', '-Wswitch-default', '-Winit-self', '-Wmissing-include-dirs', '-Wundef', '-Waggregate-return', '-Wmissing-format-attribute', '-Wnested-externs', '-Wstrict-prototypes']
		ctx.env.CFLAGS += ['-ggdb']

		# FIXME
		ctx.env.CFLAGS.remove('-Wmissing-include-dirs')

		ctx.define('G_DISABLE_DEPRECATED', 1)
	else:
		ctx.env.CFLAGS += ['-O2']

	ctx.define('SUSHI_VERSION', VERSION)

	ctx.write_config_header('source/config.h', remove = False)

def build (ctx):
	# maki
	ctx.program(
		source = ctx.path.ant_glob('source/*.c', excl = 'source/sushi-remote.c'),
		target = 'source/maki',
		use = ['GIO', 'GLIB', 'GMODULE', 'GOBJECT', 'GTHREAD', 'NICE'],
		includes = ['source'],
		defines = ['MAKI_PLUGIN_DIRECTORY="%s"' % (Utils.subst_vars('${LIBDIR}/sushi/maki/plugins', ctx.env)),
		           'MAKI_SHARE_DIRECTORY="%s"' % (Utils.subst_vars('${DATAROOTDIR}/sushi/maki', ctx.env))]
	)

	ctx.program(
		source = 'source/sushi-remote.c',
		target = 'source/sushi-remote',
		use = ['GLIB']
	)

	# Plugins
	for plugin in ('network', 'sleep', 'upnp'):
		uselibs = ['GIO', 'GLIB', 'GMODULE', 'GOBJECT', 'GTHREAD']

		if plugin == 'upnp':
			if not ctx.env.HAVE_GUPNP and not ctx.env.HAVE_GUPNP_IGD:
				continue

			uselibs += ['GUPNP', 'GUPNP_IGD']

		ctx.shlib(
			source = ['source/plugins/%s.c' % (plugin,)],
			target = 'source/plugins/%s' % (plugin,),
			use = uselibs,
			includes = ['source'],
			install_path = '${LIBDIR}/sushi/maki/plugins'
		)

	# po
	ctx(
		features = 'intltool_po',
		appname = 'maki',
		podir = 'po',
		install_path = '${LOCALEDIR}'
	)

	# Documentation
	ctx.install_files('${DOCDIR}', ctx.path.ant_glob('documentation/**/*.txt'),
		relative_trick = True
	)

	# Data
	ctx.install_files('${DATAROOTDIR}/sushi/maki', 'data/dbus.xml')

	dbus_dir = ctx.env.DBUS_session_bus_services_dir

	if dbus_dir and dbus_dir.startswith(ctx.env.PREFIX):
		ctx(
			features = 'subst',
			source = 'data/de.ikkoku.sushi.service.in',
			target = 'data/de.ikkoku.sushi.service',
			install_path = '${DBUS_session_bus_services_dir}'
		)

	for man in ('maki.1', 'sushi-remote.1'):
		ctx(
			features = 'subst',
			source = 'data/%s.in' % (man,),
			target = 'data/%s' % (man,),
			install_path = None,
			SUSHI_VERSION = VERSION
		)

	ctx.add_group()

	for man in ('maki.1', 'sushi-remote.1'):
		ctx(
			source = 'data/%s' % (man,),
			target = 'data/%s.gz' % (man),
			rule = '${GZIP} -c ${SRC} > ${TGT}',
			install_path = '${MANDIR}/man1'
		)
