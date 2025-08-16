project('CXBTool', 'cpp',
  version: '0.1',
  default_options: [
    'cpp_std=c++17',
    'b_lto=true',             # optional: link-time optimization
    'b_staticpic=true',       # make static libraries PIC
    'b_ndebug=if-release'     # disable asserts in release
  ]
)

zlib_dep = dependency('zlib', fallback: ['zlib', 'zlib_dep'], static: true)
lzo_dep = dependency('lzo2', fallback: ['lzo2', 'lzo2_dep'], static: true)
tinyxml2_dep = dependency('tinyxml2', fallback: ['tinyxml2', 'tinyxml2_dep'], static: true)

srcs = files(
  'src/main.cpp',
  'src/cxb_parser.cpp',
  'src/xml_loader.cpp',
)

executable('CXBTool',
  srcs,
  include_directories: include_directories('.'),
  dependencies: [
    zlib_dep,
    tinyxml2_dep,
    lzo_dep,
  ],
  gui_app: false,
  link_args: [
    '-static',             # link everything statically
    '-static-libgcc',
    '-static-libstdc++',
  ],
)

