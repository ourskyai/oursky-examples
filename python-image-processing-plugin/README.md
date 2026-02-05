# Python Image Processing Plugin for Edge Controller

## Overview
This example demonstrates a suitable container image for use as an image processing plugin with the Edge Controller device. The example demonstrates the API schema needed to interoperate successfully with the Edge Controller. It also shows how to open a FITS file produced by the Edge Controller, manipulate it, and pass it back to the Edge Controller. Finally, the example shows how to generate custom image processing consumers in other programming languages using the provided OpenAPI specification.

## OpenAPI Spec

View the OpenAPI spec at [consumer/openapi.yaml](consumer/openapi.yaml)

## Custom Processing Logic

Add your custom processing logic in [default_api_impl.py](consumer/generated/src/openapi_server/impl/default_api_impl.py)

What does the example consumer do?

The example algorithm takes the image passed by the Edge Controller and applies a black box to the center of the image. This is an example to demonstrate the format of the FITS file as well as a simple form of image manipulation.


### Building and Running the Example
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

## Managing the plugin on your Edge Controller Device

To manage the custom image processing pipeline you will need the IP address of your Edge Controller on your local network.

Only one plugin may be registered with the system at a time. To upload a new plugin, first delete the old one.

A plugin may be activated or deactivated using the PUT update request below.


```shell
# Create One Plugin
# The friendly name can be anything you want, but we recommend using the archive name and version.
curl -i -X POST 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/image-processing/plugin?name=<YOUR_ARCHIVE_NAME_FROM_INSTRUCTIONS_ABOVE>&active=true&sha256Checksum=<YOUR_CHECKSUM_FROM_INSTRUCTIONS_ABOVE>' \
  -H 'Authorization: Bearer raw-id-token' \
  -H 'Content-Type: application/octet-stream' \
  -T /<FULL_PATH_TO_IMAGE>/<YOUR_ARCHIVE_FILENAME>.tar


# Get All Plugins
curl -X GET 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/image-processing/plugins/' \
  -H 'Authorization: Bearer raw-id-token'

# Get One Plugin By ID
curl -i -X PUT 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/image-processing/plugin' \
  -H 'Authorization: Bearer raw-id-token' \
  -H 'Content-Type: application/json' \
  -d '{
    "pluginId": "<THE_ID_OF_YOUR_PLUGIN_FROM_THE_GET_ALL_ENDPOINT_ABOVE>",
    "active": false
  }'


# Update One Plugin By ID
# You can use this feature to enable or disable your plugin, while leaving the image in place for quick use later.
# A value of active: true will instruct the Edge Controller to pass images to the plugin
# A value of active: false will instruct the Edge Controller to skip passing images to the plugin
curl -i -X PUT 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/image-processing/plugin' \
  -H 'Authorization: Bearer raw-id-token' \
  -H 'Content-Type: application/json' \
  -d '{
    "pluginId": "<THE_ID_OF_YOUR_PLUGIN_FROM_THE_GET_ALL_ENDPOINT_ABOVE>",
    "active": false
  }'

# Delete One Container By ID
curl -X DELETE 'http://<YOUR_EDGE_CONTROLLER_IP>:9080/node-platform/v1/image-processing/plugin' \
  -H 'Authorization: Bearer raw-id-token' \
  -H 'Content-Type: application/json' \
  -d '{ "pluginId": "<THE_ID_OF_YOUR_PLUGIN_FROM_THE_GET_ALL_ENDPOINT_ABOVE>" }'

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


## Regenerating the Consumer and Creating New Consumers from the OpenAPI Spec

If you want to you can regenerate the consumer from scratch or in other programming languages. The scaffolding for the example is autogenerated from [consumer/openapi.yaml](consumer/openapi.yaml).

### Installing Dependencies
You will need [Python 3.13](https://www.python.org/downloads/) and [Docker Compose](https://docs.docker.com/compose/).
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

