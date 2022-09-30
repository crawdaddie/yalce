project('asimp', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])
cc = meson.get_compiler ('c')

exe = executable('asimp', 'main.c', 'audio/node.c',
  install : true,
  dependencies : [
    cc.find_library('soundio'),
    cc.find_library('m')
    ]
  )

test('basic', exe)
