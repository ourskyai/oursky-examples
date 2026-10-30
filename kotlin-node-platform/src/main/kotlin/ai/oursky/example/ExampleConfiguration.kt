package ai.oursky.example

import java.nio.file.Path
import java.time.Duration
import java.util.UUID

class ExampleConfiguration(private val environment: Map<String, String> = System.getenv()) {
    val apiToken = required("OS_API_TOKEN")
    val lineageId: UUID = UUID.fromString(required("OS_LINEAGE_ID"))
    val cloudBaseUrl = environment["OS_API_BASE_URL"] ?: "https://api.prod.oursky.ai"
    val outputDirectory: Path = Path.of(environment["OS_OUTPUT_DIRECTORY"] ?: "output")
    val timeout: Duration = Duration.ofSeconds(positiveInteger("OS_TIMEOUT_SECONDS", 120).toLong())

    val opticalTrainLabel: String get() = required("OS_OPTICAL_TRAIN_LABEL")
    val metricsWebsocketUrl: String get() = required("OS_METRICS_WEBSOCKET_URL")
    val receiverHost: String get() = required("OS_STREAM_RECEIVER_HOST")
    val receiverPort: Int get() = (environment["OS_STREAM_RECEIVER_PORT"] ?: "5000").toInt()
    val frameCount: Int get() = positiveInteger("OS_FRAME_COUNT", 5)
    val metricsDuration: Duration get() = Duration.ofSeconds(positiveInteger("OS_METRICS_SECONDS", 30).toLong())
    val exposureSeconds: Double get() = (environment["OS_EXPOSURE_SECONDS"] ?: "1.0").toDouble().also {
        require(it.isFinite() && it > 0) { "OS_EXPOSURE_SECONDS must be positive" }
    }
    val gain: Int get() = (environment["OS_GAIN"] ?: "0").toInt()
    val binning: Int get() = positiveInteger("OS_BINNING", 1)
    val offset: Int get() = (environment["OS_OFFSET"] ?: "0").toInt()
    val gainMode: Int get() = (environment["OS_GAIN_MODE"] ?: "1").toInt()

    private fun required(name: String): String = requireNotNull(environment[name]?.takeIf { it.isNotBlank() }) {
        "$name must be set"
    }

    private fun positiveInteger(name: String, default: Int): Int =
        (environment[name]?.toInt() ?: default).also { require(it > 0) { "$name must be positive" } }
}
