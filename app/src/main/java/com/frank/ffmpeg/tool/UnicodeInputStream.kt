package com.frank.ffmpeg.tool

import java.io.IOException
import java.io.InputStream
import java.io.PushbackInputStream

class UnicodeInputStream(`in`: InputStream, private val defaultEnc: String) : InputStream() {

    private var encoding: String? = null
    private var isInitialed = false
    private val internalIn: PushbackInputStream

    init {
        internalIn = PushbackInputStream(`in`, BOM_SIZE)
    }

    fun getEncoding(): String? {
        if (!isInitialed) {
            try {
                init()
            } catch (ex: IOException) {
                throw IllegalStateException("getEncoding error=" + ex.message)
            }

        }
        return encoding
    }

    /**
     * Read-ahead four bytes and check for BOM marks. Extra bytes are unread
     * back to the stream, only BOM bytes are skipped.
     */
    @Throws(IOException::class)
    private fun init() {
        if (isInitialed)
            return

        val bom = ByteArray(BOM_SIZE)
        val n: Int
        val unread: Int
        n = internalIn.read(bom, 0, bom.size)

        if (bom[0] == 0x00.toByte() && bom[1] == 0x00.toByte()
                && bom[2] == 0xFE.toByte() && bom[3] == 0xFF.toByte()) {
            encoding = "UTF-32BE"
            unread = n - 4
        } else if (bom[0] == 0xFF.toByte() && bom[1] == 0xFE.toByte()
                && bom[2] == 0x00.toByte() && bom[3] == 0x00.toByte()) {
            encoding = "UTF-32LE"
            unread = n - 4
        } else if (bom[0] == 0xEF.toByte() && bom[1] == 0xBB.toByte()
                && bom[2] == 0xBF.toByte()) {
            encoding = "UTF-8"
            unread = n - 3
        } else if (bom[0] == 0xFE.toByte() && bom[1] == 0xFF.toByte()) {
            encoding = "UTF-16BE"
            unread = n - 2
        } else if (bom[0] == 0xFF.toByte() && bom[1] == 0xFE.toByte()) {
            encoding = "UTF-16LE"
            unread = n - 2
        } else {
            encoding = defaultEnc
            unread = n
        }
        if (unread > 0)
            internalIn.unread(bom, n - unread, unread)
        isInitialed = true
    }

    @Throws(IOException::class)
    override fun close() {
        isInitialed = false
        internalIn.close()
    }

    @Throws(IOException::class)
    override fun read(): Int {
        isInitialed = true
        return internalIn.read()
    }

    companion object {
        private const val BOM_SIZE = 4
    }
}
