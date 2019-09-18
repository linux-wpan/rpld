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

foreach name : ['lua', 'lua5.3', 'lua-5.3', 'lua53']
	dep = dependency(name, version: '>=5.3', required: false)
	if dep.found()
		luadep = dep
	endif
endforeach
if not luadep.found()
	error('Couldn\'t find Lua.')
endif

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
