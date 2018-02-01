package com.frank.ffmpeg.activity;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.frank.ffmpeg.Pusher;
import com.frank.ffmpeg.R;

import java.io.File;

/**
 * 使用ffmpeg推流直播
 * Created by frank on 2018/2/2.
 */
public class PushActivity extends AppCompatActivity {

    private static final String TAG = PushActivity.class.getSimpleName();
    private static final String FILE_PATH = "storage/emulated/0/hello.flv";
    private static final String LIVE_URL = "rtmp://192.168.1.104/live/stream";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_push);

        initView();
    }

    private void initView() {
        final EditText edit_file_path = (EditText) findViewById(R.id.edit_file_path);
        final EditText edit_live_url = (EditText) findViewById(R.id.edit_live_url);
        edit_file_path.setText(FILE_PATH);
        edit_live_url.setText(LIVE_URL);

        Button btn_push_stream = (Button)findViewById(R.id.btn_push_stream);

        btn_push_stream.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //TODO 视频文件格式为flv
                final String filePath = edit_file_path.getText().toString();
                final String liveUrl = edit_live_url.getText().toString();
                Log.i(TAG, "filePath=" + filePath);
                Log.i(TAG, "liveUrl=" + liveUrl);

                if(!TextUtils.isEmpty(filePath) && !TextUtils.isEmpty(filePath)){
                    File file = new File(filePath);
                    //判断文件是否存在
                    if(file.exists()){
                        //开启子线程
                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                //开始推流
                                new Pusher().pushStream(filePath, liveUrl);
                            }
                        }).start();
                    }else {
                        Toast.makeText(PushActivity.this, "文件不存在", Toast.LENGTH_SHORT).show();
                    }
                }
            }
        });
    }

}
