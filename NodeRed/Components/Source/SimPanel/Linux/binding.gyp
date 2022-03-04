{
  "targets": [
    {
      "target_name": "l_SimPanel",
      "sources": [
        "libsimpanel.c",
        "exos_simpanel.c"
      ],
      "include_dirs": [
        '/usr/include'
      ],  
      'link_settings': {
        'libraries': [
          '-lexos-api',
          '-lzmq'
        ]
      }
    }
  ]
}
