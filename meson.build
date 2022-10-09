project('simple-synth', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])
cc = meson.get_compiler ('c')

exe = executable('simple-synth', 'src/main.c', 'src/audio/node.c', 'src/user_ctx.c', 'src/audio/graph.c',
  install : true,
  dependencies : [
    cc.find_library('jack'),
    cc.find_library('m'),
    cc.find_library('SDL2'),
    cc.find_library('libsndfile')
    ]
  )

test('basic', exe)
