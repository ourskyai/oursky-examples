package ai.oursky.example

import ai.oursky.protobuf.metrics.V1Metric
import ai.oursky.protobuf.metrics.V2MetricBatch
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import java.time.Duration
import java.time.Instant
import java.util.Base64
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException
import ai.oursky.platform.api.apis.DefaultApi as PlatformApi

class MetricsStreamExample(
    private val configuration: ExampleConfiguration,
    private val platform: PlatformApi,
) {
    fun run() {
        val websocketUrl = configuration.metricsWebsocketUrl
        val duration = configuration.metricsDuration
        val edgeToken = platform.v1GenerateEdgeToken(lineageId = configuration.lineageId).token
        val mapper = jacksonObjectMapper()
        val authorized = CompletableFuture<Unit>()
        val finished = CompletableFuture<Unit>()
        val httpClient = OkHttpClient.Builder().readTimeout(Duration.ZERO).build()
        val websocket = httpClient.newWebSocket(
            Request.Builder().url(websocketUrl).build(),
            object : WebSocketListener() {
                override fun onOpen(webSocket: WebSocket, response: Response) {
                    webSocket.send(
                        mapper.writeValueAsString(
                            mapOf("type" to "authorization", "bearer" to edgeToken, "headers" to emptyMap<String, String>()),
                        ),
                    )
                }

                override fun onMessage(webSocket: WebSocket, text: String) {
                    try {
                        val message = mapper.readTree(text)
                        when (message.path("type").asText()) {
                            "authorization" -> {
                                check(message.path("authorized").asBoolean()) { "EC WebSocket authorization failed" }
                                val subject = "metrics.oursky.${configuration.lineageId}.>"
                                check(webSocket.send(mapper.writeValueAsString(mapOf("type" to "subscribe", "subject" to subject))))
                                println("Subscribed to $subject")
                                authorized.complete(Unit)
                            }
                            "nats" -> printMetrics(
                                subject = message.path("subject").asText(),
                                payload = Base64.getDecoder().decode(message.path("data").asText()),
                            )
                        }
                    } catch (error: Exception) {
                        authorized.completeExceptionally(error)
                        finished.completeExceptionally(error)
                        webSocket.close(1000, "Example failed")
                    }
                }

                override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                    authorized.completeExceptionally(t)
                    finished.completeExceptionally(t)
                }

                override fun onClosing(webSocket: WebSocket, code: Int, reason: String) {
                    webSocket.close(code, reason)
                    val error = IllegalStateException("EC WebSocket closed: $code $reason")
                    authorized.completeExceptionally(error)
                    finished.completeExceptionally(error)
                }
            },
        )
        try {
            authorized.get(15, TimeUnit.SECONDS)
            try {
                finished.get(duration.toMillis(), TimeUnit.MILLISECONDS)
            } catch (_: TimeoutException) {
                println("Finished listening for metrics")
            }
        } finally {
            websocket.close(1000, "Example complete")
            websocket.cancel()
            httpClient.dispatcher.executorService.shutdown()
            httpClient.connectionPool.evictAll()
        }
    }

    private fun printMetrics(subject: String, payload: ByteArray) {
        when (subject.substringAfterLast('.')) {
            "v1Metric" -> {
                val metric = V1Metric.parseFrom(payload)
                val measuredAt = Instant.ofEpochSecond(metric.measuredAt.seconds, metric.measuredAt.nanos.toLong())
                println("$measuredAt ${metric.name}=${metric.value} ${metric.type} ${metric.tagsList}")
            }
            "v2Metrics" -> V2MetricBatch.parseFrom(payload).seriesList.forEach { series ->
                series.pointsList.forEach { point ->
                    val measuredAt = Instant.ofEpochSecond(point.measuredAt.seconds, point.measuredAt.nanos.toLong())
                    println("$measuredAt ${series.name}=${point.value} ${series.type} ${series.tagsList}")
                }
            }
        }
    }
}
