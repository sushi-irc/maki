#!/usr/bin/env python
# coding: utf-8

from waflib import Utils

APPNAME = 'maki'
VERSION = '1.4.0'

top = '.'
out = 'build'

def options (ctx):
	ctx.load('compiler_c')

	ctx.add_option('--debug', action='store_true', default=False, help='Enable debug mode')

	ctx.add_option('--miniupnpc', action='store', default='/usr', help='Use MiniUPnPc')

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
		atleast_version = '2.40',
		uselib_store = 'GIO'
	)

	ctx.check_cfg(
		package = 'glib-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.40',
		uselib_store = 'GLIB'
	)

	ctx.check_cfg(
		package = 'gmodule-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.40',
		uselib_store = 'GMODULE'
	)

	ctx.check_cfg(
		package = 'gobject-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.40',
		uselib_store = 'GOBJECT'
	)

	ctx.check_cfg(
		package = 'gthread-2.0',
		args = ['--cflags', '--libs'],
		atleast_version = '2.40',
		uselib_store = 'GTHREAD'
	)

	ctx.check_cfg(
		package = 'nice',
		args = ['--cflags', '--libs'],
		mandatory = False
	)

	# FIXME https://bugzilla.gnome.org/show_bug.cgi?id=667494

	#ctx.check_cfg(
	#	package = 'gupnp-1.0',
	#	args = ['--cflags', '--libs'],
	#	atleast_version = '0.17.2',
	#	uselib_store = 'GUPNP',
	#	mandatory = False
	#)

	#ctx.check_cfg(
	#	package = 'gupnp-igd-1.0',
	#	args = ['--cflags', '--libs'],
	#	atleast_version = '0.2.0',
	#	uselib_store = 'GUPNP_IGD',
	#	mandatory = False
	#)

	ctx.check_cc(
		header_name = 'miniupnpc.h',
		lib = 'miniupnpc',
		includes = ['%s/include/miniupnpc' % (ctx.options.miniupnpc,)],
		libpath = ['%s/lib' % (ctx.options.miniupnpc,)],
		uselib_store = 'MINIUPNPC',
		define_name = 'HAVE_MINIUPNPC',
		mandatory = False
	)

	if ctx.env.HAVE_MINIUPNPC:
		ctx.check_cc(
			fragment = '''
			#include <stdlib.h>

			#include <miniupnpc.h>
			#include <upnpcommands.h>

			int main (void)
			{
				struct UPNPUrls miniupnpc_urls = { 0 };
				struct IGDdatas miniupnpc_datas;

				UPNP_AddPortMapping(miniupnpc_urls.controlURL, miniupnpc_datas.first.servicetype, "42", "42", "127.0.0.1", "sushi IRC", "TCP", NULL, "600");

				return 0;
			}
			''',
			msg = 'Checking for MiniUPnPc 1.6',
			use = ['MINIUPNPC'],
			define_name = 'HAVE_MINIUPNPC_16',
			mandatory = False
		)

	if ctx.options.debug:
		ctx.env.CFLAGS += ['-pedantic', '-Wall', '-Wextra']
		ctx.env.CFLAGS += ['-Wno-missing-field-initializers', '-Wno-unused-parameter', '-Wold-style-definition', '-Wdeclaration-after-statement', '-Wmissing-declarations', '-Wmissing-prototypes', '-Wredundant-decls', '-Wmissing-noreturn', '-Wshadow', '-Wpointer-arith', '-Wcast-align', '-Wwrite-strings', '-Winline', '-Wformat-nonliteral', '-Wformat-security', '-Wswitch-enum', '-Wswitch-default', '-Winit-self', '-Wmissing-include-dirs', '-Wundef', '-Waggregate-return', '-Wmissing-format-attribute', '-Wnested-externs', '-Wstrict-prototypes']
		ctx.env.CFLAGS += ['-ggdb']

		ctx.define('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_2_40', quote = False)
	else:
		ctx.env.CFLAGS += ['-O2']

	ctx.define('SUSHI_VERSION', VERSION)

	# FIXME remove=false
	ctx.write_config_header('source/config.h', remove = False)

def build (ctx):
	# maki
	ctx.program(
		source = ctx.path.ant_glob('source/*.c', excl = 'source/remote.c'),
		target = 'source/maki',
		use = ['GIO', 'GLIB', 'GMODULE', 'GOBJECT', 'GTHREAD', 'NICE'],
		includes = ['source'],
		defines = ['MAKI_PLUGIN_DIRECTORY="%s"' % (Utils.subst_vars('${LIBDIR}/maki/plugins', ctx.env)),
		           'MAKI_SHARE_DIRECTORY="%s"' % (Utils.subst_vars('${DATADIR}/maki', ctx.env))]
	)

	ctx.program(
		source = 'source/remote.c',
		target = 'source/maki-remote',
		use = ['GLIB']
	)

	# Plugins
	for plugin in ('sleep', 'upnp'):
		uselibs = ['GIO', 'GLIB', 'GMODULE', 'GOBJECT', 'GTHREAD']

		if plugin == 'upnp':
			if not ctx.env.HAVE_GUPNP and not ctx.env.HAVE_GUPNP_IGD and not ctx.env.HAVE_MINIUPNPC:
				continue

			uselibs += ['GUPNP', 'GUPNP_IGD', 'MINIUPNPC']

		ctx.shlib(
			source = ['source/plugins/%s.c' % (plugin,)],
			target = 'source/plugins/%s' % (plugin,),
			use = uselibs,
			includes = ['source'],
			install_path = '${LIBDIR}/maki/plugins'
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
	ctx.install_files('${DATADIR}/maki', 'data/dbus.xml')

	ctx(
		features = 'subst',
		source = 'data/de.ikkoku.sushi.service.in',
		target = 'data/de.ikkoku.sushi.service',
		install_path = '${DATADIR}/dbus-1/services'
	)

	for man in ('maki.1', 'maki-remote.1'):
		ctx(
			features = 'subst',
			source = 'data/%s.in' % (man,),
			target = 'data/%s' % (man,),
			install_path = None,
			SUSHI_VERSION = VERSION
		)

	ctx.add_group()

	for man in ('maki.1', 'maki-remote.1'):
		ctx(
			source = 'data/%s' % (man,),
			target = 'data/%s.gz' % (man),
			rule = '${GZIP} -c ${SRC} > ${TGT}',
			install_path = '${MANDIR}/man1'
		)
