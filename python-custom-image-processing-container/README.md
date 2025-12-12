# Python Custom Image Processing Container for Edge Controller

## Overview
This example demonstrates how to generate a Python server from an OpenAPI spec, run it in a container, and perform custom image processing on the Edge Controller device.

## OpenAPI Spec

View the OpenAPI spec at [consumer/openapi.yaml](consumer/openapi.yaml)

## Custom Processing Logic

Add your custom processing logic in [default_api_impl.py](consumer/generated/src/openapi_server/impl/default_api_impl.py)

## Usage

### Installing Dependencies
You will need Python 3.13 and Docker Compose.
```shell
make install
```

This installs the Python OpenAPI generator in a virtual environment.

### Generating a Stub
```shell
make generate
```

This generates the server stub and Docker Compose configuration.

**Note:** This will overwrite the included example Dockerfile, Docker Compose, and requirements.txt. Use this if you need to start from scratch or generate for a new language.
However, retain the originals for comparison. Your container must have a default entrypoint defined.

To see other available languages for server stub generation, use the list command and scroll to the `SERVER generators` section:
```shell
source venv/bin/activate
openapi-generator-cli list
```

To generate for other languages, modify the command from the [Makefile](Makefile):
```shell
. venv/bin/activate && openapi-generator-cli generate -i consumer/openapi.yaml -g <INSERT-YOUR-DESIRED-SERVER-TYPE> -o consumer/generated
```
**Note**: The generator does not always include the latest language or package versions in the generated code. Take liberty in using your preferred newer language version or dependencies after initial generation.


### Running the Example
```shell
make run
```

This builds and runs the container with volumes for receiving and writing data to and from the primary image processing pipeline on the Edge Controller.