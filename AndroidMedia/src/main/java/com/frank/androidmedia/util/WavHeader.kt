package com.frank.androidmedia.util

import java.io.ByteArrayOutputStream
import java.io.IOException

/**
 * The header of wave format
 * @author frank
 * @date 2022/3/22
 */
internal class WavHeader {
    var riffID = charArrayOf('R', 'I', 'F', 'F')
    @JvmField
    var riffSize = 0
    var riffType = charArrayOf('W', 'A', 'V', 'E')
    var formatID = charArrayOf('f', 'm', 't', ' ')
    @JvmField
    var formatSize = 0
    @JvmField
    var formatTag: Short = 0
    @JvmField
    var numChannels: Short = 0
    @JvmField
    var sampleRate = 0
    @JvmField
    var avgBytesPerSec = 0
    @JvmField
    var blockAlign: Short = 0
    @JvmField
    var bitsPerSample: Short = 0
    var dataID = charArrayOf('d', 'a', 't', 'a')
    @JvmField
    var dataSize = 0

    @get:Throws(IOException::class)
    val header: ByteArray
        get() {
            val bos = ByteArrayOutputStream()
            writeChar(bos,  riffID)
            writeInt(bos,   riffSize)
            writeChar(bos,  riffType)
            writeChar(bos,  formatID)
            writeInt(bos,   formatSize)
            writeShort(bos, formatTag.toInt())
            writeShort(bos, numChannels.toInt())
            writeInt(bos,   sampleRate)
            writeInt(bos,   avgBytesPerSec)
            writeShort(bos, blockAlign.toInt())
            writeShort(bos, bitsPerSample.toInt())
            writeChar(bos,  dataID)
            writeInt(bos,   dataSize)
            bos.flush()
            val r = bos.toByteArray()
            bos.close()
            return r
        }

    @Throws(IOException::class)
    private fun writeShort(bos: ByteArrayOutputStream, s: Int) {
        val data = ByteArray(2)
        data[1] = (s shl 16 shr 24).toByte()
        data[0] = (s shl 24 shr 24).toByte()
        bos.write(data)
    }

    @Throws(IOException::class)
    private fun writeInt(bos: ByteArrayOutputStream, n: Int) {
        val buf = ByteArray(4)
        buf[3] = (n shr 24).toByte()
        buf[2] = (n shl 8 shr 24).toByte()
        buf[1] = (n shl 16 shr 24).toByte()
        buf[0] = (n shl 24 shr 24).toByte()
        bos.write(buf)
    }

    private fun writeChar(bos: ByteArrayOutputStream, id: CharArray) {
        for (c in id) {
            bos.write(c.toInt())
        }
    }
}