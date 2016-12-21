{
  "targets": [
    {
      "target_name": "sspi-client",
      "sources": [
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "conditions": [
        [
          "OS==\"win\"",
          {
            "sources": [
              "src_native/sspi_client.cpp",
              "src_native/utils.cpp",
              "src_native/sspi_impl.cpp"
            ]
          }
        ]
      ]
    }
  ]
}
