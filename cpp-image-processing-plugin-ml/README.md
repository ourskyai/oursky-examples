# Python SNPE

# Overview

In order to make use of Qualcomm's integrated Tensor Processing unit (TPU) on the Observable Space Edge Controller, users need to make use of the Qualcomm [SNPE Framework](https://docs.qualcomm.com/doc/80-63442-10/topic/SNPE_general_overview.html)

This example demonstrates how to construct an image processing plugin that leverages ML accelerators on the Edge Controller device. It implements a C++ HTTP Server that will handles webhook event nofications and process images taken.

## OpenAPI Spec

View the OpenAPI spec at [consumer/openapi.yaml](consumer/openapi.yaml). The consumer implements this API to handle image notifications.

## Prerequisites

1. Download and install Qualcom SNPE framework. Place it in the `third-party/snpe-sdk/` directory.


### Building and Running the Example

This builds and runs the container with volumes for receiving and writing data to and from the primary image processing pipeline on the Edge Controller.
1. Run the `scripts/setup.sh` script. This script will download a demo classification model, and convert it to DLC format.
The `./scripts/setup.sh` file will have output two DLC models in the `SNPE_ROOT/examples/Models/InceptionV3/dlc/` folder.

You will see the Non Quantized and Quantized versions of the inception model as shown below.
```
$  ls third-party/snpe-sdk/2.28.2.241116/examples/Models/InceptionV3/dlc
inception_v3.dlc  inception_v3_quantized.dlc
```
2. *Optional*: Run the server locally. The following variables will be required.
```bash
LABELS_PATH=<PATH_TO_REPO>/oursky-examples/cpp-image-processing-plugin-ml/prerequisites/imagenet_slim_labels.txt
MODEL_PATH=<PATH_TO_REPO>/oursky-examples/cpp-image-processing-plugin-ml/third-party/snpe-sdk/2.28.2.241116/examples/Models/InceptionV3/dlc/inception_v3.dlc
```
3. *Optional:*  Test the API
```bash
scripts/test_api.sh
```
4. Build the runtime dockerfile. `scripts/build.sh`. The Dockerfile at `docker/Dockerfile.snpe-run` copies in relevant libraries to run SNPE in docker.
5. Deploy the model to device.



## Customizing the example

### Customizing Models

Qualcomm SNPE documentation will provide the best reference for how to create a DLC model, but overall the steps are as follows.

1. Create an ONNX Model. 
2. Convert this model to the SNPE DLC format. 
NOTE: Ensure that your ONNX Model only uses compatible onnx operations. [SNPE ONNX Reference](https://docs.qualcomm.com/nav/home/general_supported_onnx_ops.html?product=1601111740010412)
2. Prepare quantization input list from calibration images
3. Quantize the model
4. Optimize for HTP on the target SoC
5. Build an arm64 Docker image and save as `models/<model-name>-snpe-run.tar`

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