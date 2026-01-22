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


## Archiving the container image for upload to the Edge Controller device.

```shell
# List available docker images
docker image ls
                                                                                                i Info →   U  In Use
IMAGE                                                               ID             DISK USAGE   CONTENT SIZE   EXTRA
custom-image-processing:latest                                      129b00b9ab08       1.35GB             0B    U
something-elsee:latest                                              397c01d3ce17       1.50GB             0B    U

# Copy the ID of your image processing image

# Archive the image to the .tar file of your choice
# You may use any name, but the file should be a .tar archive
docker save <your-image-id> >  custom-image-proc.tar

# Generate the sha256sum hash for the file. This is needed for upload.
sha256sum custom-image-proc.tar
39542b25225a60553aa15b6185b0e3ff26fa1e70005e31dcb1c53ec0d6cdd006  custom-image-proc.tar
```

## Managing the container on your Edge Controller Device

To manage the custom image processing pipeline you will need to the IP address of your Edge Controller on your local network.



```shell
# Create One Container
# THe friendly name can be anything you want, but we reocmmend using the archive name and version.
curl -i -X POST 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/custom-image-processing/container?imageFriendlyName=<YOUR_ARCHIVE_NAME_FROM_INSTRUCTIONS_ABOVE>>&active=true&sha256Checksum=<YOUR_CHECKSUM_FROM_INSTRUCTIONS_ABOVE>' \
  -H 'Authorization: Bearer raw-id-token' \
  -H 'Content-Type: application/octet-stream' \
  -T /<FULL_PAHT_TO_IMAGE>/<YOUR_ARCHIVE_FILENAME>.tar


# Get All Containers

curl -X GET 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/custom-image-processing/containers' \
  -H 'Authorization: Bearer raw-id-token'

# Get One Container By ID

curl -X GET 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/custom-image-processing/container?id=<THE_ID_OF_YOUR_CONTAINER_FROM_THE_GET_ALL_ENDPOINT_ABOVE>' \
  -H 'Authorization: Bearer raw-id-token'


# Update One Container By ID
# You can use this feature to enable or disable custom image procesing.
# A value of active: true will instruct the Edge Controller to pass images to the container
# A value of active: false will instruct th Edge Controller to skip passing images to the container
curl -i -X PUT 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/custom-image-processing/container?id=<THE_ID_OF_YOUR_CONTAINER_FROM_THE_GET_ALL_ENDPOINT_ABOVE>' \
  -H 'Authorization: Bearer raw-id-token' \
  -H 'Content-Type: application/json' \
  -d '{
    "active": false
  }'

# Delete One Container By ID
curl -X DELETE 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/custom-image-processing/container?id=<THE_ID_OF_YOUR_CONTAINER_FROM_THE_GET_ALL_ENDPOINT_ABOVE>' \
  -H 'Authorization: Bearer raw-id-token'

# Take One image
curl -i -X POST 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/camera/capture-image' \
  -H 'Authorization: Bearer raw-id-token' \
  -H 'Content-Type: application/json' \
  -d '{
    "exposure": 1.5,
    "gain": 100,
    "binning": 1,
    "offset": 10,
    "gainMode": 1
  }'
```

