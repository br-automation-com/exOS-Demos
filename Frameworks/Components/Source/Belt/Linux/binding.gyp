{
  "targets": [
    {
      "target_name": "l_Belt",
      "sources": [
        "libbelt.c",
        "exos_belt.c"
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
