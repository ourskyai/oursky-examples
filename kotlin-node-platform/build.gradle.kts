plugins {
    kotlin("jvm") version "2.2.20"
    application
    id("com.google.protobuf") version "0.9.1"
}

repositories {
    mavenCentral()
    maven {
        url = uri("https://maven.pkg.github.com/ourskyai/api")
        credentials {
            username = "oursky"
            password = System.getenv("OS_GITHUB_PACKAGES_TOKEN")
        }
        content { includeGroup("ai.oursky") }
    }
}

dependencies {
    implementation("ai.oursky:c2.api:1.4.693")
    implementation("ai.oursky:node_platform.api:1.4.693")
    implementation("ai.oursky:platform.api:1.4.693")
    implementation("com.squareup.okhttp3:okhttp:5.1.0")
    implementation("com.fasterxml.jackson.module:jackson-module-kotlin:2.20.0")
    implementation("com.google.protobuf:protobuf-java:3.21.12")
    implementation("com.google.flatbuffers:flatbuffers-java:2.0.0")
    testImplementation(kotlin("test-junit5"))
    testImplementation("com.squareup.okhttp3:mockwebserver:5.1.0")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher:1.10.2")
}

kotlin { jvmToolchain(21) }
application { mainClass.set("ai.oursky.example.MainKt") }
protobuf { protoc { artifact = "com.google.protobuf:protoc:3.21.12" } }
tasks.test { useJUnitPlatform() }
