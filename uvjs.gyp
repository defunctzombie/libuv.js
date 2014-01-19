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
      ],

      'sources': [
        'common.gypi',
        'src/uvjs.cpp',
      ],

      'defines': [
        'ARCH="<(target_arch)"',
        'PLATFORM="<(OS)"',
      ],

      'direct_dependent_settings': {
        'include_dirs': [
          'src',
        ],
      },
    },
    {
      'target_name': 'uvjs',
      'type': 'executable',

      'include_dirs': [
        'src',
      ],

      'sources': [
        'common.gypi',
        'test/shell/main.cpp'
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

    {
      'target_name': 'uv',
      'type': 'executable',

      'dependencies': [
        'js2c#host',
      ],

      'include_dirs': [
        'src',
        '<(SHARED_INTERMEDIATE_DIR)', # for natives.h
      ],

      'sources': [
        'common.gypi',
        'uv/main.cpp'
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

    {
      'target_name': 'js2c',
      'type': 'none',
      'toolsets': ['host'],
      'actions': [
        {
          'action_name': 'js2c',
          'inputs': [
            'uv/lib/bootstrap.js',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/natives.h',
          ],
          'action': [
            '<(python)',
            'tools/js2c.py',
            '<@(_outputs)',
            '<@(_inputs)',
          ],
        },
      ],
    }, # end node_js2c
  ] # end targets
}
