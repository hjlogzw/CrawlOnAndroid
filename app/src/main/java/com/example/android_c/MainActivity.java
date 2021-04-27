package com.example.android_c;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.ContentUris;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.text.Editable;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Properties;
import java.util.Random;

import com.chaquo.python.Kwarg;
import com.chaquo.python.PyObject;
import com.chaquo.python.Python;
import com.chaquo.python.android.AndroidPlatform;
import com.sun.mail.imap.IMAPFolder;
import com.sun.mail.imap.IMAPStore;
import javax.mail.Address;
import javax.mail.Authenticator;
import javax.mail.Flags;
import javax.mail.Folder;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.NoSuchProviderException;
import javax.mail.PasswordAuthentication;
import javax.mail.Session;
import javax.mail.Store;
import javax.mail.search.FlagTerm;

public class MainActivity extends AppCompatActivity implements View.OnClickListener{


    public static final int CHOOSE_PHOTO = 1;
    public static final int SET_PATH = 2;
    public static int result;

    private EditText edt_source_file;
    private EditText edt_dir_file;
    private EditText edt_serect_file;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-test");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        findViewById(R.id.mail).setOnClickListener(this);
        findViewById(R.id.weibo).setOnClickListener(this);
        findViewById(R.id.forum).setOnClickListener(this);
        findViewById(R.id.instant_msg).setOnClickListener(this);



        // Example of a call to a native method
        // TextView tv = findViewById(R.id.sample_text);
        // tv.setText(stringFromJNITest());
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native int fileProcess(String inFilePath, String outFilePath, String newFile,
                                  int sliceNum, float reductionRatio);

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.mail:
                Intent intentMail = new Intent(MainActivity.this,
                        MailPublishActivity.class);
                startActivity(intentMail);
                Toast.makeText(getApplicationContext(), "使用邮箱发布",
                        Toast.LENGTH_SHORT).show();
                break;
            case R.id.weibo:
                Intent intentWeibo = new Intent(MainActivity.this,
                        WeiboPublishActivity.class);
                startActivity(intentWeibo);
                Toast.makeText(this, "使用微博发布",
                        Toast.LENGTH_SHORT).show();
                break;
            case R.id.instant_msg:

                break;
            case R.id.forum:

                break;
            default:
                break;
        }
    }

    private void test() throws MessagingException {
        Properties prop = new Properties();
        prop.put("mail.store.protocol", "imap");
        prop.put("mail.imap.host", "imap.qq.com");

        Session session = Session.getInstance(prop);
        session.setDebug(true);
        IMAPStore store = (IMAPStore) session.getStore("imap"); // 使用imap会话机制，连接服务器
        store.connect("2524617616@qq.com", "ydanxjuzverodjhi");
        IMAPFolder folder = (IMAPFolder) store.getFolder("INBOX"); // 收件箱
        folder.open(Folder.READ_WRITE);

        FlagTerm ft = new FlagTerm(new Flags(Flags.Flag.SEEN), false);

        // 获取未读邮件
        Message[] messages = folder.search(ft);

        /*
         * Folder.READ_ONLY：只读权限 Folder.READ_WRITE：可读可写（可以修改邮件的状态）
         */


        System.out.println("未读邮件数: " + folder.getUnreadMessageCount());

        // 由于POP3协议无法获知邮件的状态,所以下面得到的结果始终都是为0
        System.out.println("删除邮件数: " + folder.getDeletedMessageCount());
        System.out.println("新邮件: " + folder.getNewMessageCount());

        // 获得收件箱中的邮件总数
        System.out.println("邮件总数: " + folder.getMessageCount());

        // 得到收件箱中的所有邮件,并解析

        //parseMessage(messages);

        // 得到收件箱中的所有邮件并且删除邮件
        // deleteMessage(messages);

        // 释放资源
        if (folder != null)
            folder.close(true);
        if (store != null)
            store.close();
    }

    private void delete_files(String filePath) {
        File file = new File(filePath);
        File[] files = file.listFiles();  // 获取指定路径下的文件或文件夹
        for (int i = 0; i < files.length; i++) {
            if (files[i].isFile()){//如果是文件直接删除
                File deleFile = new File(files[i].getPath());
                Log.d("delete filePath -->> ", deleFile.getPath());
                deleFile.delete();
            }
            else {
                    //如果是文件夹再次迭代进里面找到指定文件路径
                    File[] myfile = files[i].listFiles();
                    for (int d = 0; d < myfile.length; d++) {
                        File photoFile = new File(myfile[d].getPath());
                        photoFile.delete();
                    }
                }
        }
    }

    private void choose_source_file() {
        Intent intent = new Intent("android.intent.action.GET_CONTENT");
        intent.setType("image/*");
        startActivityForResult(intent, CHOOSE_PHOTO);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {

        if (grantResults.length <= 0 || grantResults[0] != PackageManager.PERMISSION_GRANTED){
            Toast.makeText(this, "You denied the permission",
                    Toast.LENGTH_SHORT).show();
        }else
        {
            switch (requestCode){
                case 1:
                    choose_source_file();
                    break;
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        switch (requestCode){
            case CHOOSE_PHOTO:
                if (resultCode == RESULT_OK){
                    // 判断手机系统版本号
                    if (Build.VERSION.SDK_INT >= 19){  // 4.4及以上
                        handleImageOnKitKat(data);
                    }else{
                        handleImageBeforeKitKat(data);
                    }
                }
                break;
            default:
                break;
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    private void handleImageOnKitKat(Intent data) {
        String imagePath  = null;
        Uri uri = data.getData();
        if (DocumentsContract.isDocumentUri(this, uri)){
            String docId = DocumentsContract.getDocumentId(uri);
            if ("com.android.providers.media.documents".equals(uri.getAuthority())){
                String id = docId.split(":")[1];
                String selection = MediaStore.Images.Media._ID + "=" + id;
                imagePath = getImagePath(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,selection);
            }else if ("com.android.providers.downloads.documents".equals(uri.getAuthority())){
                Uri contentUri = ContentUris.withAppendedId(Uri
                        .parse("content://downloads/public_downloads"), Long.valueOf(docId));
                imagePath = getImagePath(contentUri, null);
            }
        }else if ("content".equalsIgnoreCase(uri.getScheme())){
            imagePath = getImagePath(uri, null);
        }else if ("file".equalsIgnoreCase(uri.getScheme())){
            imagePath = uri.getPath();
        }
        edt_source_file.setText(imagePath);
        //displayImage(imagePath);
    }

    private void handleImageBeforeKitKat(Intent data) {
        Uri uri = data.getData();
        String imagePath = getImagePath(uri, null);
        edt_source_file.setText(imagePath);
    }

    private String getImagePath(Uri uri, String selection){
        String path = null;
        Cursor cursor = (Cursor) getContentResolver().query(uri, null, selection, null, null);
        if (cursor != null){
            if (cursor.moveToFirst()){
                path = cursor.getString(cursor.getColumnIndex(MediaStore.Images.Media.DATA));
            }
            cursor.close();
        }
        return path;
    }

}
