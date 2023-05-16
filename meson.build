project('fermat', 'cpp')
executable('fermat', 'expr.cpp',
  cpp_args: '-fsanitize=address',
  link_args: '-fsanitize=address',
  include_directories: include_directories(
    'vendor',
    'vendor/termcolor/include/termcolor'
  )
)
