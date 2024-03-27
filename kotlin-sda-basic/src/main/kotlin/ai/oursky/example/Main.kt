package ai.oursky.example

import okhttp3.Interceptor
import okhttp3.OkHttpClient
import java.time.Duration
import java.time.OffsetDateTime
import java.util.UUID

fun main(args: Array<String>) {
    // initialization
    val apiToken = System.getenv("OURSKY_API_TOKEN") ?: throw Exception("OURSKY_API_TOKEN is not set")
    val bearerTokenInterceptor = Interceptor { chain ->
        val originalRequest = chain.request()
        val modifiedRequest =
            originalRequest.newBuilder()
                .addHeader("Authorization", "Bearer $apiToken")
                .build()
        chain.proceed(modifiedRequest)
    }
    val defaultTimeout = Duration.ofSeconds(60)
    val httpClient = OkHttpClient()
        .newBuilder()
        .addInterceptor(bearerTokenInterceptor)
        .callTimeout(defaultTimeout)
        .readTimeout(defaultTimeout)
        .writeTimeout(defaultTimeout)
        .build()
    val sdaClient = ai.oursky.sda.api.apis.DefaultApi(client = httpClient)

    // logic
    val targetId = UUID.fromString("6e801835-4aae-4b78-9741-f10fbab472db") // ISS
    val allPotentials = sdaClient.v1GetSatellitePotentials(
        until = OffsetDateTime.now().plusDays(2),
        satelliteTargetId = targetId,
    ).ifEmpty {
        throw Exception("No upcoming observation windows found for target ID $targetId")
    }

    allPotentials.sortedBy {
        it.firstObservableTime
    }.forEach { potential ->
        println("first observable time: ${potential.firstObservableTime}")
        println("last observable time: ${potential.lastObservableTime}")
        println()
    }
}
