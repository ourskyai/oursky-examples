package ai.oursky.example

fun main(arguments: Array<String>) {
    val command = arguments.singleOrNull()
    require(command in setOf("c2-capture", "continuous-capture", "metrics")) {
        "Usage: ./gradlew run --args=<c2-capture|continuous-capture|metrics>"
    }
    val configuration = ExampleConfiguration()
    when (command) {
        "c2-capture" -> ExampleClients(configuration = configuration).use { clients ->
            C2CaptureExample(configuration = configuration, c2 = clients.c2, nodePlatform = clients.nodePlatform).run()
        }
        "continuous-capture" -> ExampleClients(configuration = configuration).use { clients ->
            ContinuousCaptureExample(configuration = configuration, nodePlatform = clients.nodePlatform).run()
        }
        "metrics" -> ExampleClients(configuration = configuration).use { clients ->
            MetricsStreamExample(configuration = configuration, platform = clients.platform).run()
        }
    }
}
