package ai.oursky.example

import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import java.net.Socket
import java.net.URI
import java.nio.file.Files
import java.nio.file.Path
import java.util.UUID
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertContains
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import okhttp3.mockwebserver.Dispatcher
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import okhttp3.mockwebserver.RecordedRequest
import org.junit.jupiter.api.io.TempDir

class CaptureExamplesTest {
    @TempDir
    lateinit var outputDirectory: Path

    private val lineageId = UUID.randomUUID()
    private val taskId = UUID.randomUUID()
    private val imageId = UUID.randomUUID()
    private val mapper = jacksonObjectMapper()

    @Test
    fun `C2 polls its exact task and downloads its resulting image`() {
        MockWebServer().use { server ->
            server.enqueue(json("""{"tasks":[${task("EXECUTING")}]}"""))
            server.enqueue(json("""{"tasks":[${task("SUCCEEDED", """{"id":"$imageId"}""")}],"offset":0}"""))
            server.enqueue(json(imageMetadata()))
            server.enqueue(MockResponse().setBody("captured FITS bytes"))
            val configuration = configuration(server)
            ExampleClients(configuration).use { clients ->
                val savedImage = C2CaptureExample(configuration, clients.c2, clients.nodePlatform).run()
                assertEquals("captured FITS bytes", Files.readString(savedImage))
            }

            val capture = requireNotNull(server.takeRequest(1, TimeUnit.SECONDS))
            assertEquals("/c2/v1/camera/capture-image", capture.path)
            assertEquals("Bearer test-token", capture.getHeader("Authorization"))
            val selectors = mapper.readTree(capture.body.readUtf8()).path("opticalTrains")
            assertEquals(lineageId.toString(), selectors[0].path("lineageId").asText())
            assertEquals("astro", selectors[0].path("opticalTrainLabel").asText())

            val poll = requireNotNull(server.takeRequest(1, TimeUnit.SECONDS))
            assertEquals("/c2/v1/tasks", poll.path)
            assertEquals(taskId.toString(), mapper.readTree(poll.body.readUtf8()).path("ids")[0].asText())
            val metadata = requireNotNull(server.takeRequest(1, TimeUnit.SECONDS))
            assertEquals(imageId.toString(), metadata.requestUrl?.queryParameter("id"))
            val download = requireNotNull(server.takeRequest(1, TimeUnit.SECONDS))
            assertEquals(imageId.toString(), download.requestUrl?.queryParameter("id"))
            assertEquals("FITS", download.requestUrl?.queryParameter("format"))
        }
    }

    @Test
    fun `failed C2 capture reports details without downloading an unrelated image`() {
        MockWebServer().use { server ->
            server.enqueue(json("""{"tasks":[${task("FAILED")}]}"""))
            val configuration = configuration(server)
            ExampleClients(configuration).use { clients ->
                val failure = assertFailsWith<IllegalStateException> {
                    C2CaptureExample(configuration, clients.c2, clients.nodePlatform).run()
                }
                assertContains(failure.message.orEmpty(), "camera unavailable")
            }
            assertEquals(1, server.requestCount)
        }
    }

    @Test
    fun `continuous capture saves a frame and stops capture and stream`() {
        exerciseContinuousCapture(validFrame = true)
        assertEquals(1, Files.list(outputDirectory).use { it.count() }.toInt())
    }

    @Test
    fun `receiver failure still stops continuous capture and stream`() {
        exerciseContinuousCapture(validFrame = false)
        assertEquals(0, Files.list(outputDirectory).use { it.count() }.toInt())
    }

    private fun exerciseContinuousCapture(validFrame: Boolean) {
        MockWebServer().use { server ->
            var receiverSocket: Socket? = null
            server.dispatcher = object : Dispatcher() {
                override fun dispatch(request: RecordedRequest): MockResponse {
                    when {
                        request.path.orEmpty().contains("start-image-stream") -> {
                            val receiverUrl = URI(mapper.readTree(request.body.clone().readUtf8()).path("streamReceiverUrl").asText())
                            receiverSocket = Socket(receiverUrl.host, receiverUrl.port)
                        }
                        request.path.orEmpty().contains("/capture-images-continuously") -> {
                            val payload = if (validFrame) ImageFrameReceiverTest.imageFrame(byteArrayOf(12, 34)) else byteArrayOf(0, 0, 0, 0)
                            requireNotNull(receiverSocket).getOutputStream().write(payload)
                        }
                    }
                    return json("{}")
                }
            }
            val configuration = configuration(server)
            try {
                ExampleClients(configuration).use { clients ->
                    val example = ContinuousCaptureExample(configuration, clients.nodePlatform)
                    if (validFrame) example.run() else assertFailsWith<IllegalArgumentException> { example.run() }
                }
            } finally {
                receiverSocket?.close()
            }
            val requests = List(4) { requireNotNull(server.takeRequest(1, TimeUnit.SECONDS)) }
            assertContains(requests[0].path.orEmpty(), "/start-image-stream")
            assertContains(requests[1].path.orEmpty(), "/capture-images-continuously")
            assertContains(requests[2].path.orEmpty(), "/stop-capturing-images-continuously")
            assertContains(requests[3].path.orEmpty(), "/stop-image-stream")
            assertEquals(false, mapper.readTree(requests[1].body.readUtf8()).path("enableFileSaving").asBoolean())
            for (request in requests) {
                assertEquals(lineageId.toString(), request.requestUrl?.queryParameter("lineageId"))
            }
            assertEquals("astro", requests[1].requestUrl?.queryParameter("opticalTrainLabel"))
            assertEquals("astro", requests[2].requestUrl?.queryParameter("opticalTrainLabel"))
        }
    }

    private fun configuration(server: MockWebServer): ExampleConfiguration = ExampleConfiguration(
        mapOf(
            "OS_API_TOKEN" to "test-token",
            "OS_LINEAGE_ID" to lineageId.toString(),
            "OS_OPTICAL_TRAIN_LABEL" to "astro",
            "OS_API_BASE_URL" to server.url("/").toString(),
            "OS_STREAM_RECEIVER_HOST" to "127.0.0.1",
            "OS_STREAM_RECEIVER_PORT" to "0",
            "OS_FRAME_COUNT" to "1",
            "OS_TIMEOUT_SECONDS" to "5",
            "OS_OUTPUT_DIRECTORY" to outputDirectory.toString(),
        ),
    )

    private fun json(body: String): MockResponse = MockResponse().setHeader("Content-Type", "application/json").setBody(body)

    private fun task(status: String, result: String = "null"): String = """
        {"lineageId":"$lineageId","task":{"id":"$taskId","createdAt":"2026-10-29T00:00:00Z","updatedAt":"2026-10-29T00:00:00Z","status":"$status","type":"CAPTURE_IMAGE","statusDetail":"camera unavailable","result":$result}}
    """.trimIndent()

    private fun imageMetadata(): String = """
        {"id":"$imageId","binning":1,"createdAt":"2026-10-29T00:00:00Z","updatedAt":"2026-10-29T00:00:00Z","exposure":1.0,"gain":0,"offset":0}
    """.trimIndent()
}
