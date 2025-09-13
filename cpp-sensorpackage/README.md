### Sensor Package Sample Client

Contains multiple c++ sample apps to showcase various functoinality talkign to Space Optics API over HTTP. 
- capture single image synchoronsouly and print metadat
- stream frames via tcp socket

## Requirements
- Linux with a C++20 compiler
- CMake ≥ 3.22
- Ninja
- Boost (system, serialization)
- Internet access (for CMake FetchContent of FlatBuffers & nlohmann_json)

## Quick install (Ubuntu/Debian)
```
$ sudo apt-get update
$ sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  libboost-system-dev \
  libboost-serialization-dev
```

FlatBuffers (library + flatc) and nlohmann_json are fetched automatically by CMake via FetchContent.

## Build

From the repo root:
```
$ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build
```

## Run 

From the repo root:

```
$ ./build/capture-single-image 
Querying connected cameras...
Connected cameras (2):
  - id: "Dell-XPS-webcam-bd0d2b90-cbc0-4631-9c09-e99be11a245b", name: "V4L2 Camera"
  - id: "QHY-92abf791-0e67-4a9e-a289-35e9acc0415f", name: "QHY268M"
Using camera: QHY-92abf791-0e67-4a9e-a289-35e9acc0415f
Capturing single image...
Capture results:
  - payloadBytes: 52479280
  - metadata.width: 6252
  - metadata.height: 4176
  - metadata.image_id: 4cfd3527-f78a-451f-8bfc-dd1b535e9c02
```

```
$ ./build/stream-frames 
Querying connected cameras...
Connected cameras (2):
  - id: "Dell-XPS-webcam-d9e4a3f5-20b4-4c08-8ef5-ee4028c7db54", name: "V4L2 Camera"
  - id: "QHY-f51c54cf-b561-4bb8-b363-921c13776cd0", name: "QHY268M"
Using camera: QHY-f51c54cf-b561-4bb8-b363-921c13776cd0
Starting continuous exposure...
Continuous exposure started successfully!
Response: {"success":true,"errorMessage":null}
Stream URL: tcp://127.0.0.1:35841
Streaming started successfully! Waiting for TCP connection...
Sender connected from 127.0.0.1:52556
  - metadata.width: 3126
  - metadata.height: 2088
  - metadata.image_id: c482bd4b-82d7-4ab5-bf1e-f2a6eb236fe7
Frame #0 : 13316760 bytes
  - metadata.width: 3126
  - metadata.height: 2088
  - metadata.image_id: 32899f2f-2f3d-4bf7-8067-030dd81fe9db
Frame #1 : 13316760 bytes
  - metadata.width: 3126
  - metadata.height: 2088
  - metadata.image_id: 0943d325-acc8-47bd-a052-6c9c9cd86ddb
Frame #2 : 13316760 bytes
  - metadata.width: 3126
  - metadata.height: 2088
  - metadata.image_id: 2049b7dc-a9b8-4559-aa78-fce6535b9dd7
Frame #3 : 13316760 bytes
```
