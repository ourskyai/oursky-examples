package ai.oursky.example

import okhttp3.OkHttpClient
import ai.oursky.c2.api.apis.DefaultApi as C2Api
import ai.oursky.node_platform.api.apis.DefaultApi as NodePlatformApi
import ai.oursky.platform.api.apis.DefaultApi as PlatformApi

class ExampleClients(configuration: ExampleConfiguration) : AutoCloseable {
    private val httpClient = OkHttpClient.Builder()
        .callTimeout(configuration.timeout)
        .readTimeout(configuration.timeout)
        .addInterceptor { chain ->
            chain.proceed(
                chain.request().newBuilder()
                    .header("Authorization", "Bearer ${configuration.apiToken}")
                    .build(),
            )
        }
        .build()

    val c2 = C2Api(basePath = configuration.cloudBaseUrl.trimEnd('/'), client = httpClient)
    val nodePlatform = NodePlatformApi(basePath = configuration.cloudBaseUrl.trimEnd('/'), client = httpClient)
    val platform = PlatformApi(basePath = configuration.cloudBaseUrl.trimEnd('/'), client = httpClient)

    override fun close() {
        httpClient.dispatcher.executorService.shutdown()
        httpClient.connectionPool.evictAll()
    }
}
