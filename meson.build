project('fermat', 'cpp')
executable('fermat',
  'source/fermat.cpp',
  'source/expr.cpp',
  'source/operand.cpp',
  'source/operation_impl.cpp',
  cpp_args: [ '-std=c++20', '-fsanitize=address' ],
  link_args: '-fsanitize=address',
  include_directories: include_directories(
    'vendor',
    'vendor/termcolor/include/termcolor'
  )
)
