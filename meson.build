project('simple-synth', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])
cc = meson.get_compiler ('c')
# sdl2 = dependency('sdl2')

exe = executable('simple-synth', 'src/main.c', 'src/audio/node.c',
  install : true,
  dependencies : [
    cc.find_library('soundio'),
    cc.find_library('m'),
    cc.find_library('SDL2')
    ]
  )

test('basic', exe)
