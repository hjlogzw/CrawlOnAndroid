package com.example.android_c;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.EditText;

import com.chaquo.python.Kwarg;
import com.chaquo.python.PyObject;
import com.chaquo.python.Python;
import com.chaquo.python.android.AndroidPlatform;

import java.util.ArrayList;
import java.util.List;

public class WeiboPublishActivity extends AppCompatActivity implements View.OnClickListener{
    private static final String TAG = "PythonOnAndroid";
    private static String slices_path = Environment.getExternalStorageDirectory().getAbsolutePath()
            +"/Yx/slice_file/";
    private EditText edt_num;
    private EditText edt_save;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_weibo_publish);

        edt_num = findViewById(R.id.edt_num);
        edt_save = findViewById(R.id.edt_save);
        findViewById(R.id.btn_start).setOnClickListener(this);

        initPython();

    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.btn_start:
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        callPythonCode();
                    }
                }).start();
                break;
            default:
                break;
        }
    }

    // 初始化Python环境
    void initPython(){
        if (! Python.isStarted()) {
            Python.start(new AndroidPlatform(this));
        }
    }
    // 调用python代码
    void callPythonCode(){
        Python py = Python.getInstance();

        PyObject obj2 = py.getModule("callPyLib").callAttr("test_run",
                1,'y', slices_path);
    }
}
