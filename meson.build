project('rpld', 'c')
compiler = meson.get_compiler('c')

evdep = dependency('ev', required: false)
if evdep.found()
	evdep = [ libev ]
elif compiler.has_header('ev.h') and compiler.has_function('ev_time', args: ['-lev'])
	evdep = [ compiler.find_library('ev') ]
else
	error('Couldn\'t find libev header / library')
endif

luadep = dependency('lua5.3')
mnldep = dependency('libmnl')

srcs = files(
	'rpld.c',
	'config.c',
	'socket.c',
	'buffer.c',
	'recv.c',
	'send.c',
	'helpers.c',
	'process.c',
	'netlink.c',
	'dag.c',
	'log.c',
)

executable('rpld', srcs, dependencies : [ evdep, luadep, mnldep ])

# vim: syntax=python
