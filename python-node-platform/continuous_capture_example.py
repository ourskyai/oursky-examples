import logging
import socket
from contextlib import ExitStack

from ourskyai_node_platform_api import (
    DefaultApi as NodePlatformApi,
    V1CaptureImagesContinuouslyRequest,
    V1StartImageStreamRequest,
    V1StopCapturingImagesContinuouslyRequest,
)

from example_configuration import ExampleConfiguration
from image_frame_receiver import ImageFrameReceiver


class ContinuousCaptureExample:
    def __init__(
        self, *, configuration: ExampleConfiguration, node_platform: NodePlatformApi,
    ) -> None:
        self.configuration = configuration
        self.node_platform = node_platform

    def run(self) -> None:
        configuration = self.configuration
        optical_train_label = configuration.optical_train_label
        frame_count = configuration.frame_count
        receiver_host = configuration.receiver_host
        request = V1CaptureImagesContinuouslyRequest(
            enable_file_saving=False, **configuration.camera_settings,
        )
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as receiver:
            receiver.bind(("0.0.0.0", configuration.receiver_port))
            receiver.listen(1)
            receiver.settimeout(configuration.timeout_seconds)
            receiver_url = f"tcp://{receiver_host}:{receiver.getsockname()[1]}"
            print(f"Listening on {receiver.getsockname()[1]}; EC will connect to {receiver_url}", flush=True)
            cleanup = ExitStack()
            primary_error = None
            try:
                self.node_platform.v1_start_image_stream(
                    v1_start_image_stream_request=V1StartImageStreamRequest(
                        stream_receiver_url=receiver_url,
                    ),
                    lineage_id=configuration.lineage_id,
                    _request_timeout=configuration.timeout_seconds,
                )
                cleanup.callback(
                    self.node_platform.v1_stop_image_stream,
                    lineage_id=configuration.lineage_id,
                    _request_timeout=configuration.timeout_seconds,
                )
                self.node_platform.v1_capture_images_continuously(
                    v1_capture_images_continuously_request=request,
                    lineage_id=configuration.lineage_id,
                    optical_train_label=optical_train_label,
                    _request_timeout=configuration.timeout_seconds,
                )
                cleanup.callback(
                    self.node_platform.v1_stop_capturing_images_continuously,
                    v1_stop_capturing_images_continuously_request=V1StopCapturingImagesContinuouslyRequest(),
                    lineage_id=configuration.lineage_id,
                    optical_train_label=optical_train_label,
                    _request_timeout=configuration.timeout_seconds,
                )
                connection, _ = receiver.accept()
                with connection:
                    connection.settimeout(configuration.timeout_seconds)
                    image_frame_receiver = ImageFrameReceiver(connection=connection)
                    for _ in range(frame_count):
                        image_frame_receiver.save_next_frame(output_directory=configuration.output_directory)
            except BaseException as error:
                primary_error = error
                raise
            finally:
                try:
                    cleanup.close()
                except Exception:
                    if primary_error is None:
                        raise
                    logging.exception("Failed to stop capture or streaming after the original error")
