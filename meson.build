project('simple-synth', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])
cc = meson.get_compiler ('c')
search_paths = [
  '/usr/lib', '/usr/local/lib', '/opt/homebrew/lib' ]

exe = executable('simple-synth', 'src/main.c', 'src/user_ctx.c', 'src/audio/graph.c', 'src/queue.c',
  install : true,
  dependencies : [
    cc.find_library('jack'),
    cc.find_library('m'),
    cc.find_library('SDL2', dirs: search_paths),
	  cc.find_library(
      'sndfile',
      dirs: search_paths
    )]
  )

test('basic', exe)
