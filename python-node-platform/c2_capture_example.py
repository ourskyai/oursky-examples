import time
from uuid import UUID

from ourskyai_c2_api import (
    C2V1CaptureImageRequest,
    C2V1GetTasksRequest,
    C2V1OpticalTrainSelector,
    DefaultApi as C2Api,
    V1Task,
    V1TaskStatus,
    V2CaptureImageRequest,
)

from example_configuration import ExampleConfiguration


class C2CaptureExample:
    def __init__(self, *, configuration: ExampleConfiguration, c2: C2Api) -> None:
        self.configuration = configuration
        self.c2 = c2

    def run(self) -> UUID:
        configuration = self.configuration
        submitted = self.c2.c2_v1_capture_image(
            c2_v1_capture_image_request=C2V1CaptureImageRequest(
                optical_trains=[C2V1OpticalTrainSelector(
                    lineage_id=configuration.lineage_id,
                    optical_train_label=configuration.optical_train_label,
                )],
                request=V2CaptureImageRequest(**configuration.camera_settings),
            ),
            _request_timeout=configuration.timeout_seconds,
        )
        (submitted_task,) = submitted.tasks
        print(f"Submitted capture task {submitted_task.task.id}", flush=True)
        completed_task = self._await_capture(submitted_task=submitted_task.task)
        if not completed_task.result or "id" not in completed_task.result:
            raise ValueError("Capture result has no image id")
        image_id = UUID(str(completed_task.result["id"]))
        print(f"Captured image {image_id}; file stored locally on the node")
        return image_id

    def _await_capture(self, *, submitted_task: V1Task) -> V1Task:
        deadline = time.monotonic() + self.configuration.timeout_seconds
        task = submitted_task
        while True:
            print(f"Task {task.id}: {task.status.value}", flush=True)
            if task.status == V1TaskStatus.SUCCEEDED:
                return task
            if task.status in {V1TaskStatus.FAILED, V1TaskStatus.CANCELLED, V1TaskStatus.MISSED}:
                raise RuntimeError(f"Capture {task.id} ended with {task.status.value}: {task.status_detail}")
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(
                    f"Timed out waiting for {task.id}; the cloud task may still be running. "
                    "Inspect or cancel it through C2."
                )
            time.sleep(min(1.0, remaining))
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                continue
            response = self.c2.c2_v1_get_tasks(
                c2_v1_get_tasks_request=C2V1GetTasksRequest(
                    lineage_ids=[self.configuration.lineage_id], ids=[submitted_task.id],
                ),
                _request_timeout=remaining,
            )
            (returned_task,) = response.tasks
            task = returned_task.task
