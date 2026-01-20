project('CXBTool', 'cpp',
  version: '0.1',
  default_options: [
    'cpp_std=c++17',
    'b_lto=true',             # optional: link-time optimization
    'b_staticpic=true',       # make static libraries PIC
    'b_ndebug=if-release'     # disable asserts in release
  ]
)

lzo_dep = dependency('lzo2', fallback: ['lzo2', 'lzo2_dep'], static: true)
pugixml_dep = dependency('pugixml', fallback: ['pugixml', 'pugixml_dep'], static: true)

srcs = files(
  'src/main.cpp',
  'src/cxb_parser.cpp',
)

executable('CXBTool',
  srcs,
  include_directories: include_directories('.'),
  dependencies: [
    lzo_dep,
    pugixml_dep,
  ],
  gui_app: false,
  link_args: [
    '-static',             # link everything statically
    '-static-libgcc',
    '-static-libstdc++',
  ],
)

