{
  "targets": [
    {
      "target_name": "sspi-client",
      "sources": [
        "src_native/sspi_client.cpp",
        "src_native/sspi_impl.cpp",
        "src_native/utils.cpp"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ]
    }
 ]
}
