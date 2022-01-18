{
  "targets": [
    {
      "target_name": "l_Sequence",
      "sources": [
        "libsequence.c",
        "exos_sequence.c"
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
