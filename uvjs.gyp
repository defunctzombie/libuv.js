{
  'variables': {
    'v8_use_snapshot%': 'true',
    # Turn off -Werror in V8
    # See http://codereview.chromium.org/8159015
    'werror': '',
  },

  'targets': [
    {
      'target_name': 'libuvjs',
      'type': 'static_library',

      'include_dirs': [
        'src',
        'vendor/uv/include',
        'vendor/v8/include',
        '<(SHARED_INTERMEDIATE_DIR)' # for node_natives.h
      ],

      'sources': [
        'common.gypi',
        'src/uvjs.cpp',
      ],

      'defines': [
        'ARCH="<(target_arch)"',
        'PLATFORM="<(OS)"',
      ],
    },
    {
      'target_name': 'uvjs',
      'type': 'executable',

      'include_dirs': [
        'src',
        '<(SHARED_INTERMEDIATE_DIR)' # for node_natives.h
      ],

      'sources': [
        'common.gypi',
        'test/shell.cpp',
      ],

      'defines': [
        'ARCH="<(target_arch)"',
        'PLATFORM="<(OS)"',
      ],

      'dependencies': [
        'vendor/v8/tools/gyp/v8.gyp:v8',
        'vendor/uv/uv.gyp:libuv',
        'libuvjs',
      ],
    },
  ] # end targets
}
