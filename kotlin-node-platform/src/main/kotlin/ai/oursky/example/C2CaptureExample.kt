package ai.oursky.example

import ai.oursky.c2.api.models.C2V1CaptureImageRequest
import ai.oursky.c2.api.models.C2V1GetTasksRequest
import ai.oursky.c2.api.models.C2V1OpticalTrainSelector
import ai.oursky.c2.api.models.V1Task
import ai.oursky.c2.api.models.V1TaskStatus
import ai.oursky.c2.api.models.V2CaptureImageRequest
import ai.oursky.node_platform.api.models.ImageFileFormat
import java.nio.file.Files
import java.nio.file.Path
import java.util.UUID
import ai.oursky.c2.api.apis.DefaultApi as C2Api
import ai.oursky.node_platform.api.apis.DefaultApi as NodePlatformApi

class C2CaptureExample(
    private val configuration: ExampleConfiguration,
    private val c2: C2Api,
    private val nodePlatform: NodePlatformApi,
) {
    fun run(): Path {
        val submittedTask = c2.c2V1CaptureImage(
            c2V1CaptureImageRequest = C2V1CaptureImageRequest(
                opticalTrains = listOf(
                    C2V1OpticalTrainSelector(
                        lineageId = configuration.lineageId,
                        opticalTrainLabel = configuration.opticalTrainLabel,
                    ),
                ),
                request = V2CaptureImageRequest(
                    exposure = configuration.exposureSeconds,
                    gain = configuration.gain,
                    binning = configuration.binning,
                    offset = configuration.offset,
                    gainMode = configuration.gainMode,
                ),
            ),
        ).tasks.single().task
        println("Submitted capture task ${submittedTask.id}")
        val completedTask = awaitCapture(submittedTask = submittedTask)
        val imageId = UUID.fromString(requireNotNull(completedTask.result?.get("id")) { "Capture result has no image id" }.toString())
        println(nodePlatform.v1GetImage(id = imageId, lineageId = configuration.lineageId))
        Files.createDirectories(configuration.outputDirectory)
        val destination = configuration.outputDirectory.resolve("$imageId.fits")
        val downloadedFile = nodePlatform.v1GetImageFile(
            id = imageId,
            lineageId = configuration.lineageId,
            format = ImageFileFormat.FITS,
        )
        try {
            Files.copy(downloadedFile.toPath(), destination)
        } finally {
            downloadedFile.delete()
        }
        println("Saved $destination")
        return destination
    }

    private fun awaitCapture(submittedTask: V1Task): V1Task {
        val deadline = System.nanoTime() + configuration.timeout.toNanos()
        var task = submittedTask
        while (true) {
            println("Task ${task.id}: ${task.status}")
            when (task.status) {
                V1TaskStatus.SUCCEEDED -> return task
                V1TaskStatus.FAILED, V1TaskStatus.CANCELLED, V1TaskStatus.MISSED ->
                    error("Capture ${task.id} ended with ${task.status}: ${task.statusDetail}")
                else -> Unit
            }
            check(System.nanoTime() < deadline) {
                "Timed out waiting for ${task.id}; the cloud task may still be running. Inspect or cancel it through C2."
            }
            Thread.sleep(1_000)
            task = c2.c2V1GetTasks(
                c2V1GetTasksRequest = C2V1GetTasksRequest(
                    lineageIds = listOf(configuration.lineageId),
                    ids = listOf(task.id),
                ),
            ).tasks.single().task
        }
    }
}
