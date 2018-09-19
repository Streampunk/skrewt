{
  "targets": [
    {
      "target_name": "skrewt",
      "sources": [
        "src/skrewt_util.cc",
        "src/skrewt.cc"
      ],
      "include_dirs": [ "include" ],
      "link_settings": {
        "libraries": [ "srt.lib" ],
        "library_dirs": [ "lib/win_x64" ]
      },
      "copies": [
        {
          "destination": "build/Release",
          "files": [
            "lib/win_x64/srt.dll",
            "lib/win_x64/pthreadVC2.dll",
            "lib/win_x64/libcrypto-1_1-x64.dll"
          ]
        }
      ]
    }
  ]
}
