package ai.oursky.example

import ai.oursky.node_platform.api.models.V1CaptureImagesContinuouslyRequest
import ai.oursky.node_platform.api.models.V1StartImageStreamRequest
import ai.oursky.node_platform.api.models.V1StopCapturingImagesContinuouslyRequest
import java.net.ServerSocket
import java.net.URI
import ai.oursky.node_platform.api.apis.DefaultApi as NodePlatformApi

class ContinuousCaptureExample(
    private val configuration: ExampleConfiguration,
    private val nodePlatform: NodePlatformApi,
) {
    fun run() {
        val opticalTrainLabel = configuration.opticalTrainLabel
        val frameCount = configuration.frameCount
        ServerSocket(configuration.receiverPort).use { receiver ->
            receiver.soTimeout = configuration.timeout.toMillis().toInt()
            val receiverUrl = URI("tcp", null, configuration.receiverHost, receiver.localPort, null, null, null)
            println("Listening on ${receiver.localPort}; EC will connect to $receiverUrl")
            nodePlatform.v1StartImageStream(
                v1StartImageStreamRequest = V1StartImageStreamRequest(streamReceiverUrl = receiverUrl),
                lineageId = configuration.lineageId,
            )
            try {
                nodePlatform.v1CaptureImagesContinuously(
                    v1CaptureImagesContinuouslyRequest = V1CaptureImagesContinuouslyRequest(
                        exposure = configuration.exposureSeconds,
                        gain = configuration.gain,
                        binning = configuration.binning,
                        offset = configuration.offset,
                        gainMode = configuration.gainMode,
                        enableFileSaving = false,
                    ),
                    lineageId = configuration.lineageId,
                    opticalTrainLabel = opticalTrainLabel,
                )
                try {
                    receiver.accept().use { connection ->
                        connection.soTimeout = configuration.timeout.toMillis().toInt()
                        val imageFrameReceiver = ImageFrameReceiver(input = connection.getInputStream())
                        repeat(frameCount) {
                            imageFrameReceiver.saveNextFrame(outputDirectory = configuration.outputDirectory)
                        }
                    }
                } finally {
                    nodePlatform.v1StopCapturingImagesContinuously(
                        v1StopCapturingImagesContinuouslyRequest = V1StopCapturingImagesContinuouslyRequest(),
                        lineageId = configuration.lineageId,
                        opticalTrainLabel = opticalTrainLabel,
                    )
                }
            } finally {
                nodePlatform.v1StopImageStream(lineageId = configuration.lineageId)
            }
        }
    }
}
