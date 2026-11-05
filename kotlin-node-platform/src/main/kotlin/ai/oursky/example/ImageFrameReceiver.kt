package ai.oursky.example

import hwdaemon.ImageResult
import java.awt.image.BufferedImage
import java.awt.image.DataBufferByte
import java.awt.image.DataBufferUShort
import java.io.DataInputStream
import java.io.InputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.file.Files
import java.nio.file.Path
import javax.imageio.ImageIO

class ImageFrameReceiver(input: InputStream) {
    private val input = DataInputStream(input)

    fun saveNextFrame(outputDirectory: Path): Path {
        val frame = readFrame()
        val image = ImageResult.getRootAsImageResult(ByteBuffer.wrap(frame).order(ByteOrder.LITTLE_ENDIAN))
        val metadata = requireNotNull(image.metadata()) { "Frame has no image metadata" }
        val imageId = java.util.UUID.fromString(requireNotNull(metadata.imageId()))
        val width = metadata.width()
        val height = metadata.height()
        val pixelCount = Math.multiplyExact(width, height)
        val rawBytes = requireNotNull(image.rawBytesAsByteBuffer()) { "Frame has no pixels" }
        val preview = when (rawBytes.remaining()) {
            pixelCount -> BufferedImage(width, height, BufferedImage.TYPE_BYTE_GRAY).also {
                rawBytes.get((it.raster.dataBuffer as DataBufferByte).data)
            }
            Math.multiplyExact(pixelCount, 2) -> BufferedImage(width, height, BufferedImage.TYPE_USHORT_GRAY).also {
                rawBytes.order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get((it.raster.dataBuffer as DataBufferUShort).data)
            }
            Math.multiplyExact(pixelCount, 3) -> BufferedImage(width, height, BufferedImage.TYPE_3BYTE_BGR).also {
                rawBytes.get((it.raster.dataBuffer as DataBufferByte).data)
            }
            else -> error("Unsupported pixel layout: ${rawBytes.remaining()} bytes for ${width}x$height")
        }
        Files.createDirectories(outputDirectory)
        val destination = outputDirectory.resolve("$imageId.png")
        check(ImageIO.write(preview, "png", destination.toFile())) { "PNG writer unavailable" }
        println("Image $imageId: ${width}x$height, ${metadata.bitDepth()} bits, ${metadata.exposureSeconds()}s; saved $destination")
        return destination
    }

    private fun readFrame(): ByteArray {
        val payloadLength = Integer.reverseBytes(input.readInt())
        require(payloadLength in 8..MAX_FRAME_BYTES) { "Invalid image frame length: $payloadLength" }
        val payload = ByteArray(payloadLength)
        input.readFully(payload)
        require(ImageResult.ImageResultBufferHasIdentifier(ByteBuffer.wrap(payload))) { "Expected OSSP image frame" }
        return payload
    }

    private companion object {
        const val MAX_FRAME_BYTES = 256 * 1024 * 1024
    }
}
