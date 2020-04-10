package com.frank.ffmpeg.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.util.ContentUtil;

/**
 * base Activity
 * Created by frank on 2019/11/2.
 */

public abstract class BaseActivity extends AppCompatActivity implements View.OnClickListener {

    private final static String TAG = BaseActivity.class.getSimpleName();

    private final static int REQUEST_CODE = 1234;
    private final static String[] permissions = new String[]{
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.CAMERA,
            Manifest.permission.RECORD_AUDIO};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && checkSelfPermission(permissions[0]) != PackageManager.PERMISSION_GRANTED
                && checkSelfPermission(permissions[1]) != PackageManager.PERMISSION_GRANTED) {
            requestPermission();
        }
        setContentView(getLayoutId());
    }

    protected void hideActionBar() {
        if (getSupportActionBar() != null) {
            getSupportActionBar().hide();
        }
    }

    private void requestPermission() {
        requestPermission(permissions);
    }

    protected void requestPermission(String[] permissions) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, REQUEST_CODE);
        }
    }

    protected void initViewsWithClick(int... viewIds) {
        for (int viewId : viewIds) {
            getView(viewId).setOnClickListener(this);
        }
    }

    @Override
    public void onClick(View v) {
        onViewClick(v);
    }

    protected void selectFile() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        this.startActivityForResult(intent, 123);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (data != null && data.getData() != null) {
            String filePath = ContentUtil.getPath(this, data.getData());
            Log.i(TAG, "filePath=" + filePath);
            onSelectedFile(filePath);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_setting, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_select:
                selectFile();
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    protected void showToast(String msg) {
        if (TextUtils.isEmpty(msg)) {
            return;
        }
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

    protected void showSelectFile() {
        showToast(getString(R.string.please_select));
    }

    protected <T extends View> T getView(int viewId) {
        return (T) findViewById(viewId);
    }

    abstract int getLayoutId();

    abstract void onViewClick(View view);

    abstract void onSelectedFile(String filePath);

}
