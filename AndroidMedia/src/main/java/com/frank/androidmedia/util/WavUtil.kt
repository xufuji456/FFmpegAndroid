package com.frank.androidmedia.util

import android.util.Log
import java.io.*

/**
 * Convert pcm to wav
 *
 * @author frank
 * @date 2022/3/22
 */
object WavUtil {

    fun makePCMToWAVFile(pcmPath: String?, wavPath: String?, deletePcmFile: Boolean): Boolean {
        val buffer: ByteArray
        val file = File(pcmPath)
        if (!file.exists()) {
            return false
        }
        val len = file.length().toInt()
        val header = WavHeader()
        header.riffSize       = len + (44 - 8)
        header.formatSize     = 16
        header.bitsPerSample  = 16
        header.numChannels    = 2
        header.formatTag      = 0x0001
        header.sampleRate     = 44100
        header.blockAlign     = (header.numChannels * header.bitsPerSample / 8).toShort()
        header.avgBytesPerSec = header.blockAlign * header.sampleRate
        header.dataSize       = len
        val h: ByteArray = try {
            header.header
        } catch (e1: IOException) {
            e1.message?.let { Log.e("WavUtil", it) }
            return false
        }
        if (h.size != 44) return false
        val dstFile = File(wavPath)
        if (dstFile.exists()) dstFile.delete()
        try {
            buffer = ByteArray(1024 * 4)
            val inStream: InputStream
            val ouStream: OutputStream
            ouStream = BufferedOutputStream(FileOutputStream(wavPath))
            ouStream.write(h, 0, h.size)
            inStream = BufferedInputStream(FileInputStream(file))
            var size = inStream.read(buffer)
            while (size != -1) {
                ouStream.write(buffer)
                size = inStream.read(buffer)
            }
            inStream.close()
            ouStream.close()
        } catch (e: IOException) {
            e.message?.let { Log.e("WavUtil", it) }
            return false
        }
        if (deletePcmFile) {
            file.delete()
        }
        Log.i("WavUtil", "makePCMToWAVFile  success...")
        return true
    }
}