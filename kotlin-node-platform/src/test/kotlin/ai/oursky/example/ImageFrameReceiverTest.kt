package ai.oursky.example

import com.google.flatbuffers.FlatBufferBuilder
import hwdaemon.ImageMetadata
import hwdaemon.ImageResult
import java.io.ByteArrayInputStream
import java.io.EOFException
import java.io.FilterInputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.file.Files
import java.nio.file.Path
import java.util.UUID
import javax.imageio.ImageIO
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.junit.jupiter.api.io.TempDir

class ImageFrameReceiverTest {
    @TempDir
    lateinit var outputDirectory: Path

    @Test
    fun `partial TCP reads preserve 16 bit mono pixels across consecutive frames`() {
        val firstFrame = imageFrame(byteArrayOf(0, 0, 0x34, 0x12), bitDepth = 16)
        val secondFrame = imageFrame(byteArrayOf(0xff.toByte(), 0xff.toByte(), 0, 0), bitDepth = 16)
        val fragmentedInput = object : FilterInputStream(ByteArrayInputStream(firstFrame + secondFrame)) {
            override fun read(buffer: ByteArray, offset: Int, length: Int): Int =
                super.read(buffer, offset, minOf(length, 3))
        }
        val receiver = ImageFrameReceiver(fragmentedInput)
        val firstImage = ImageIO.read(receiver.saveNextFrame(outputDirectory).toFile())
        val secondImage = ImageIO.read(receiver.saveNextFrame(outputDirectory).toFile())
        assertEquals(0x1234, firstImage.raster.getSample(1, 0, 0))
        assertEquals(65535, secondImage.raster.getSample(0, 0, 0))
        assertEquals(2, Files.list(outputDirectory).use { it.count() }.toInt())
    }

    @Test
    fun `eight bit mono PNG preserves pixel samples`() {
        val receiver = ImageFrameReceiver(ByteArrayInputStream(imageFrame(byteArrayOf(12, 127))))
        val image = ImageIO.read(receiver.saveNextFrame(outputDirectory).toFile())
        assertEquals(12, image.raster.getSample(0, 0, 0))
        assertEquals(127, image.raster.getSample(1, 0, 0))
    }

    @Test
    fun `BGR frame is written with the correct PNG colors`() {
        val frame = imageFrame(byteArrayOf(0, 0, 0xff.toByte(), 0xff.toByte(), 0, 0))
        val receiver = ImageFrameReceiver(input = ByteArrayInputStream(frame))
        val image = ImageIO.read(receiver.saveNextFrame(outputDirectory = outputDirectory).toFile())
        assertEquals(0xff0000, image.getRGB(0, 0) and 0xffffff)
        assertEquals(0x0000ff, image.getRGB(1, 0) and 0xffffff)
    }

    @Test
    fun `truncated frame fails before writing an image`() {
        val truncated = imageFrame(byteArrayOf(1, 2)).dropLast(1).toByteArray()
        assertFailsWith<EOFException> {
            ImageFrameReceiver(ByteArrayInputStream(truncated)).saveNextFrame(outputDirectory)
        }
        assertEquals(0, Files.list(outputDirectory).use { it.count() }.toInt())
    }

    @Test
    fun `invalid frame length is rejected before allocating payload`() {
        val prefix = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(Int.MAX_VALUE).array()
        assertFailsWith<IllegalArgumentException> {
            ImageFrameReceiver(ByteArrayInputStream(prefix)).saveNextFrame(outputDirectory)
        }
    }

    companion object {
        fun imageFrame(pixels: ByteArray, bitDepth: Int = 8): ByteArray {
            val builder = FlatBufferBuilder()
            val imageId = builder.createString(UUID.randomUUID().toString())
            ImageMetadata.startImageMetadata(builder)
            ImageMetadata.addWidth(builder, 2)
            ImageMetadata.addHeight(builder, 1)
            ImageMetadata.addBitDepth(builder, bitDepth)
            ImageMetadata.addExposureSeconds(builder, 1.0)
            ImageMetadata.addImageId(builder, imageId)
            val metadata = ImageMetadata.endImageMetadata(builder)
            val rawBytes = ImageResult.createRawBytesVector(builder, pixels)
            ImageResult.startImageResult(builder)
            ImageResult.addRawBytes(builder, rawBytes)
            ImageResult.addMetadata(builder, metadata)
            ImageResult.finishSizePrefixedImageResultBuffer(builder, ImageResult.endImageResult(builder))
            return builder.sizedByteArray()
        }
    }
}
