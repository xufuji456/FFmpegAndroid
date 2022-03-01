
package com.frank.ffmpeg.metadata;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.util.Log;

import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 *  Retrieve frame and extract metadata from an input media file.
 */
public class FFmpegMediaRetriever {

	static {
    	try {
    	    System.loadLibrary("media-handle");
        } catch (UnsatisfiedLinkError e) {
            Log.e("FFmpegMediaRetriever", "loadLibrary error=" + e.toString());
        }
    }

    // The field is accessed by native method
    private long mNativeRetriever;

    public FFmpegMediaRetriever() {
        native_init();
    	native_setup();
    }

    public void setDataSource(String path) {
        native_setDataSource(path);
    }
    
    /**
     * Sets the data source (FileDescriptor) to use. It is the caller's
     * responsibility to close the file descriptor. It is safe to do so as soon
     * as this call returns. Call this method before the rest of the methods in
     * this class. This method may be time-consuming.
     * 
     * @param fd the FileDescriptor for the file you want to play
     */
    public void setDataSource(FileDescriptor fd)
            throws IllegalArgumentException {
        if (fd == null) {
            return;
        }
        native_setDataSourceFD(fd, 0, 0x7ffffffffffffffL);
    }
    
    /**
     * Sets the data source as a content Uri. Call this method before 
     * the rest of the methods in this class. This method may be time-consuming.
     * 
     * @param context the Context to use when resolving the Uri
     * @param uri the Content URI of the data you want to play
     *
     */
    public void setDataSource(Context context, Uri uri) throws IllegalArgumentException, SecurityException {
        if (uri == null) {
            throw new IllegalArgumentException();
        }
        
        String scheme = uri.getScheme();
        if(scheme == null || scheme.equals("file")) {
            setDataSource(uri.getPath());
            return;
        }

        AssetFileDescriptor fd = null;
        try {
            ContentResolver resolver = context.getContentResolver();
            try {
                fd = resolver.openAssetFileDescriptor(uri, "r");
            } catch(FileNotFoundException e) {
                throw new IllegalArgumentException();
            }
            if (fd == null) {
                throw new IllegalArgumentException();
            }
            FileDescriptor descriptor = fd.getFileDescriptor();
            if (!descriptor.valid()) {
                throw new IllegalArgumentException();
            }
            if (fd.getDeclaredLength() < 0) {
                setDataSource(descriptor);
            } else {
                native_setDataSourceFD(descriptor, fd.getStartOffset(), fd.getDeclaredLength());
            }
            return;
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } finally {
            try {
                if (fd != null) {
                    fd.close();
                }
            } catch(IOException ioEx) {
                ioEx.printStackTrace();
            }
        }
        setDataSource(uri.toString());
    }

    public String extractMetadata(String key) {
        if (key == null || key.isEmpty()) {
            return null;
        }
        return native_extractMetadata(key);
    }

    /**
     * Call this method after setDataSource(). This method finds a
     * representative frame close to the given time position by considering
     * the given option if possible, and returns it as a bitmap. This is
     * useful for generating a thumbnail for an input data source or just
     * obtain and display a frame at the given time position.
     *
     * @param timeUs The time position where the frame will be retrieved.
     *
     * @param option a hint on how the frame is found. Use
     * {@link #OPTION_PREVIOUS_SYNC} if one wants to retrieve a sync frame
     * that has a timestamp earlier than or the same as timeUs. Use
     * {@link #OPTION_NEXT_SYNC} if one wants to retrieve a sync frame
     * that has a timestamp later than or the same as timeUs. Use
     * {@link #OPTION_CLOSEST_SYNC} if one wants to retrieve a sync frame
     * that has a timestamp closest to or the same as timeUs. Use
     * {@link #OPTION_CLOSEST} if one wants to retrieve a frame that may
     * or may not be a sync frame but is closest to or the same as timeUs.
     * {@link #OPTION_CLOSEST} often has larger performance overhead compared
     * to the other options if there is no sync frame located at timeUs.
     *
     * @return A Bitmap containing a representative video frame, which
     *         can be null, if such a frame cannot be retrieved.
     */
    public Bitmap getFrameAtTime(long timeUs, int option) {
        if (option < OPTION_PREVIOUS_SYNC ||
                option > OPTION_CLOSEST) {
            throw new IllegalArgumentException("Unsupported option: " + option);
        }

        BitmapFactory.Options bitmapOptions= new BitmapFactory.Options();
        bitmapOptions.inScaled = true;
        byte [] picture = native_getFrameAtTime(timeUs, option);
        if (picture != null) {
            return BitmapFactory.decodeByteArray(picture, 0, picture.length, bitmapOptions);
        }

        return null;
    }

    public Bitmap getFrameAtTime(long timeUs) {
        return getFrameAtTime(timeUs, OPTION_CLOSEST_SYNC);
    }

    /**
     * Call this method after setDataSource(). This method finds a
     * representative frame close to the given time position by considering
     * the given option if possible, and returns it as a bitmap. This is
     * useful for generating a thumbnail for an input data source or just
     * obtain and display a frame at the given time position.
     *
     * @param timeUs The time position where the frame will be retrieved.
     *
     * @param option a hint on how the frame is found. Use
     * {@link #OPTION_PREVIOUS_SYNC} if one wants to retrieve a sync frame
     * that has a timestamp earlier than or the same as timeUs. Use
     * {@link #OPTION_NEXT_SYNC} if one wants to retrieve a sync frame
     * that has a timestamp later than or the same as timeUs. Use
     * {@link #OPTION_CLOSEST_SYNC} if one wants to retrieve a sync frame
     * that has a timestamp closest to or the same as timeUs. Use
     * {@link #OPTION_CLOSEST} if one wants to retrieve a frame that may
     * or may not be a sync frame but is closest to or the same as timeUs.
     * {@link #OPTION_CLOSEST} often has larger performance overhead compared
     * to the other options if there is no sync frame located at timeUs.
     *
     * @return A Bitmap containing a representative video frame, which
     *         can be null, if such a frame cannot be retrieved.
     */
    public Bitmap getScaledFrameAtTime(long timeUs, int option, int width, int height) {
        if (option < OPTION_PREVIOUS_SYNC ||
                option > OPTION_CLOSEST) {
            throw new IllegalArgumentException("Unsupported option: " + option);
        }

        BitmapFactory.Options bitmapOptions = new BitmapFactory.Options();
        bitmapOptions.inScaled = true;
        byte [] picture = native_getScaleFrameAtTime(timeUs, option, width, height);
        if (picture != null) {
            return BitmapFactory.decodeByteArray(picture, 0, picture.length, bitmapOptions);
        }

        return null;
    }

    public Bitmap getScaledFrameAtTime(long timeUs, int width, int height) {
        return getScaledFrameAtTime(timeUs, OPTION_CLOSEST_SYNC, width, height);
    }

    public Bitmap getAudioThumbnail() {
        byte[] picture = native_getAudioThumbnail();
        if (picture != null) {
            return BitmapFactory.decodeByteArray(picture, 0, picture.length, null);
        }

        return null;
    }

    public void release() {
        native_release();
    }

    private native void native_setup();

    private native void native_init();

    private native void native_setDataSource(String path) throws IllegalArgumentException;

    private native void native_setDataSourceFD(FileDescriptor fd, long offset, long length) throws IllegalArgumentException;

    private native String native_extractMetadata(String key);

    private native void native_setSurface(Object surface);

    private native byte[] native_getFrameAtTime(long timeUs, int option);

    private native byte[] native_getScaleFrameAtTime(long timeUs, int option, int width, int height);

    private native byte[] native_getAudioThumbnail();

    private native void native_release();

    /**
     * This option is used with {@link #getFrameAtTime(long, int)} to retrieve
     * a sync (or key) frame associated with a data source that is located
     * right before or at the given time.
     *
     * @see #getFrameAtTime(long, int)
     */
    int OPTION_PREVIOUS_SYNC    = 0x00;
    /**
     * This option is used with {@link #getFrameAtTime(long, int)} to retrieve
     * a sync (or key) frame associated with a data source that is located
     * right after or at the given time.
     *
     * @see #getFrameAtTime(long, int)
     */
    int OPTION_NEXT_SYNC        = 0x01;
    /**
     * This option is used with {@link #getFrameAtTime(long, int)} to retrieve
     * a sync (or key) frame associated with a data source that is located
     * closest to (in time) or at the given time.
     *
     * @see #getFrameAtTime(long, int)
     */
    int OPTION_CLOSEST_SYNC     = 0x02;
    /**
     * This option is used with {@link #getFrameAtTime(long, int)} to retrieve
     * a frame (not necessarily a key frame) associated with a data source that
     * is located closest to or at the given time.
     *
     * @see #getFrameAtTime(long, int)
     */
    int OPTION_CLOSEST          = 0x03;


    /**
     * The metadata key to retrieve the original name of the file.
     */
    public static String METADATA_KEY_FILENAME = "filename";
    /**
     * The metadata key to retrieve the main language.
     */
    public static String METADATA_KEY_LANGUAGE = "language";
    /**
     * The metadata key to retrieve the name.
     */
    public static String METADATA_KEY_TITLE = "title";
    /**
     * The metadata key to retrieve the general bitrate.
     */
    public static String METADATA_KEY__BITRATE = "bitrate";
    /**
     * The metadata key to retrieve the duration in milliseconds.
     */
    public static String METADATA_KEY_DURATION = "duration";
    /**
     * The metadata key to retrieve the audio codec.
     */
    public static String METADATA_KEY_AUDIO_CODEC = "audio_codec";
    /**
     * The metadata key to retrieve the video codec.
     */
    public static String METADATA_KEY_VIDEO_CODEC = "video_codec";
    /**
     * This key retrieves the video degree of rotation.
     * The video rotation angle may be 0, 90, 180, or 270 degrees.
     */
    public static String METADATA_KEY_VIDEO_ROTATION = "rotate";
    /**
     * This metadata key retrieves the average frame rate (in frames/sec).
     */
    public static String METADATA_KEY_FRAME_RATE = "frame_rate";
    /**
     * The metadata key to retrieve the file size in bytes.
     */
    public static String METADATA_KEY_FILE_SIZE = "file_size";
    /**
     * The metadata key to retrieve the video width.
     */
    public static String METADATA_KEY_VIDEO_WIDTH = "video_width";
    /**
     * The metadata key to retrieve the video height.
     */
    public static String METADATA_KEY_VIDEO_HEIGHT = "video_height";
    /**
     * The metadata key to retrieve the mime type.
     */
    public static String METADATA_KEY_MIME_TYPE = "mime_type";
    /**
     * The metadata key to retrieve sample rate.
     */
    public static String METADATA_KEY_SAMPLE_RATE = "sample_rate";
    /**
     * The metadata key to retrieve channel count.
     */
    public static String METADATA_KEY_CHANNEL_COUNT = "channel_count";
    /**
     * The metadata key to retrieve channel layout.
     */
    public static String METADATA_KEY_CHANNEL_LAYOUT = "channel_layout";
    /**
     * The metadata key to retrieve pixel format.
     */
    public static String METADATA_KEY_PIXEL_FORMAT = "pixel_format";

}
