project('fermat', 'cpp')

cpp = meson.get_compiler('cpp')
libgccjit = cpp.find_library('gccjit')

executable('fermat',
  'source/expr.cpp',
  'source/fermat.cpp',
  'source/jit.cpp',
  'source/operand.cpp',
  'source/operation_impl.cpp',
  cpp_args: [ '-std=c++20' ],
  dependencies: [ libgccjit ],
  include_directories: include_directories(
    'vendor',
    'vendor/termcolor/include/termcolor'
  )
)
