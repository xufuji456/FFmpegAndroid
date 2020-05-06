package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.handler.FFmpegHandler;

import com.frank.ffmpeg.model.MediaBean;
import com.frank.ffmpeg.tool.JsonParseTool;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;

import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH;

/**
 * Using ffprobe to parse media format data
 * Created by frank on 2020/1/7.
 */

public class ProbeFormatActivity extends BaseActivity {

    private TextView txtProbeFormat;
    private ProgressBar progressProbe;
    private RelativeLayout layoutProbe;
    private FFmpegHandler ffmpegHandler;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_BEGIN:
                    progressProbe.setVisibility(View.VISIBLE);
                    layoutProbe.setVisibility(View.GONE);
                    break;
                case MSG_FINISH:
                    progressProbe.setVisibility(View.GONE);
                    layoutProbe.setVisibility(View.VISIBLE);
                    MediaBean result = (MediaBean) msg.obj;
                    String resultFormat = JsonParseTool.stringFormat(result);
                    if (!TextUtils.isEmpty(resultFormat) && txtProbeFormat != null) {
                        txtProbeFormat.setText(resultFormat);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    int getLayoutId() {
        return R.layout.activity_probe;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        initView();
        ffmpegHandler = new FFmpegHandler(mHandler);
    }

    private void initView() {
        progressProbe = getView(R.id.progress_probe);
        layoutProbe = getView(R.id.layout_probe);
        initViewsWithClick(R.id.btn_probe_format);
        txtProbeFormat = getView(R.id.txt_probe_format);
    }

    @Override
    public void onViewClick(View view) {
        selectFile();
    }

    @Override
    void onSelectedFile(String filePath) {
        doHandleProbe(filePath);
    }

    /**
     * use ffprobe to parse video/audio format metadata
     *
     * @param srcFile srcFile
     */
    private void doHandleProbe(final String srcFile) {
        if (!FileUtil.checkFileExist(srcFile)) {
            return;
        }
        String[] commandLine = FFmpegUtil.probeFormat(srcFile);
        if (ffmpegHandler != null) {
            ffmpegHandler.executeFFprobeCmd(commandLine);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }
    }

}
