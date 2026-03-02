# Third Party Dependencies

Download the [Qualcomm SNPE SDK](https://docs.qualcomm.com/nav/home/setup_linux.html?product=1601111740010412) and put it in this directory.

You should have a directory structure like:
```
third-party/
  snpe-sdk/
    <version>/            # e.g. 2.28.2.241116
      include/
        SNPE/
          DlSystem/
          ...
      lib/
        x86_64-linux-clang/
          libSNPE.so
        aarch64-ubuntu-gcc9.4/
          libSNPE.so
        ...
```

The CMake build system automatically discovers the SDK version directory under `third-party/snpe-sdk/` and selects the correct library for your target architecture (`x86_64-linux-clang` for x86, `aarch64-*` for ARM64). You can also set the `SNPE_ROOT` environment variable to point to the SDK location instead of placing it here.