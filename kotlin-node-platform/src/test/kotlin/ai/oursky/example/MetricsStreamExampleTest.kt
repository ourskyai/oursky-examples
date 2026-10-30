package ai.oursky.example

import ai.oursky.protobuf.metrics.MetricType
import ai.oursky.protobuf.metrics.V1Metric
import ai.oursky.protobuf.metrics.V2MetricBatch
import ai.oursky.protobuf.metrics.V2MetricPoint
import ai.oursky.protobuf.metrics.V2MetricSeries
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import com.google.protobuf.Timestamp
import java.io.ByteArrayOutputStream
import java.io.PrintStream
import java.util.Base64
import java.util.UUID
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertContains
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer

class MetricsStreamExampleTest {
    private val lineageId = UUID.randomUUID()
    private val mapper = jacksonObjectMapper()

    @Test
    fun `websocket authenticates before subscribing and decodes V1 and every V2 point`() {
        MockWebServer().use { server ->
            val incoming = LinkedBlockingQueue<String>()
            server.enqueue(MockResponse().setHeader("Content-Type", "application/json").setBody("""{"token":"node-scoped-edge-token"}"""))
            server.enqueue(
                MockResponse().withWebSocketUpgrade(object : WebSocketListener() {
                    override fun onMessage(webSocket: WebSocket, text: String) {
                        incoming.add(text)
                        when (mapper.readTree(text).path("type").asText()) {
                            "authorization" -> webSocket.send("""{"type":"authorization","authorized":true}""")
                            "subscribe" -> sendMetrics(webSocket)
                        }
                    }

                    override fun onClosing(webSocket: WebSocket, code: Int, reason: String) {
                        webSocket.close(code, reason)
                    }
                }),
            )
            val output = ByteArrayOutputStream()
            val previousOutput = System.out
            try {
                System.setOut(PrintStream(output))
                val configuration = configuration(server)
                ExampleClients(configuration).use { clients ->
                    MetricsStreamExample(configuration, clients.platform).run()
                }
            } finally {
                System.setOut(previousOutput)
            }
            val authorization = mapper.readTree(requireNotNull(incoming.poll(1, TimeUnit.SECONDS)))
            assertEquals("authorization", authorization.path("type").asText())
            assertEquals("node-scoped-edge-token", authorization.path("bearer").asText())
            val subscription = mapper.readTree(requireNotNull(incoming.poll(1, TimeUnit.SECONDS)))
            assertEquals("subscribe", subscription.path("type").asText())
            assertEquals("metrics.oursky.$lineageId.>", subscription.path("subject").asText())
            assertContains(output.toString(), "1970-01-01T00:00:10.000000123Z camera.temperature=12.5 GAUGE [source:node_controller]")
            assertContains(output.toString(), "camera.frames=2.0 COUNT")
            assertContains(output.toString(), "camera.frames=3.0 COUNT")
            val tokenRequest = requireNotNull(server.takeRequest(1, TimeUnit.SECONDS))
            assertEquals("/v1/edge-token", tokenRequest.requestUrl?.encodedPath)
            assertEquals(lineageId.toString(), tokenRequest.requestUrl?.queryParameter("lineageId"))
            assertEquals("Bearer test-token", tokenRequest.getHeader("Authorization"))
        }
    }

    @Test
    fun `rejected authorization fails without sending a subscription`() {
        MockWebServer().use { server ->
            val incoming = LinkedBlockingQueue<String>()
            server.enqueue(MockResponse().setHeader("Content-Type", "application/json").setBody("""{"token":"node-scoped-edge-token"}"""))
            server.enqueue(
                MockResponse().withWebSocketUpgrade(object : WebSocketListener() {
                    override fun onMessage(webSocket: WebSocket, text: String) {
                        incoming.add(text)
                        webSocket.send("""{"type":"authorization","authorized":false}""")
                    }

                    override fun onClosing(webSocket: WebSocket, code: Int, reason: String) {
                        webSocket.close(code, reason)
                    }
                }),
            )
            val failure = assertFailsWith<java.util.concurrent.ExecutionException> {
                val configuration = configuration(server)
                ExampleClients(configuration).use { clients ->
                    MetricsStreamExample(configuration, clients.platform).run()
                }
            }
            assertContains(failure.cause?.message.orEmpty(), "authorization failed")
            assertEquals(1, incoming.size)
        }
    }

    private fun sendMetrics(webSocket: WebSocket) {
        val timestamp = Timestamp.newBuilder().setSeconds(10).setNanos(123).build()
        val v1 = V1Metric.newBuilder()
            .setName("camera.temperature")
            .setValue(12.5)
            .setType(MetricType.GAUGE)
            .setMeasuredAt(timestamp)
            .addTags("source:node_controller")
            .build()
        val v2 = V2MetricBatch.newBuilder().addSeries(
            V2MetricSeries.newBuilder()
                .setName("camera.frames")
                .setType(MetricType.COUNT)
                .addPoints(V2MetricPoint.newBuilder().setMeasuredAt(timestamp).setValue(2.0))
                .addPoints(V2MetricPoint.newBuilder().setMeasuredAt(timestamp).setValue(3.0)),
        ).build()
        for ((suffix, payload) in listOf("v1Metric" to v1.toByteArray(), "v2Metrics" to v2.toByteArray())) {
            webSocket.send(
                mapper.writeValueAsString(
                    mapOf("type" to "nats", "subject" to "metrics.oursky.$lineageId.$suffix", "data" to Base64.getEncoder().encodeToString(payload)),
                ),
            )
        }
    }

    private fun configuration(server: MockWebServer): ExampleConfiguration = ExampleConfiguration(
        mapOf(
            "OS_API_TOKEN" to "test-token",
            "OS_LINEAGE_ID" to lineageId.toString(),
            "OS_API_BASE_URL" to server.url("/").toString(),
            "OS_METRICS_WEBSOCKET_URL" to server.url("/").toString().replaceFirst("http://", "ws://"),
            "OS_METRICS_SECONDS" to "1",
        ),
    )
}
