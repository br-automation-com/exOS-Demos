{
  "targets": [
    {
      "target_name": "l_RemoteIO",
      "sources": [
        "libremoteio.c",
        "exos_remoteio.c"
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
