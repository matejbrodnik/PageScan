package si.vicos.pagescandemo;

import androidx.core.app.ActivityCompat;
import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.hardware.camera2.CameraManager;
import android.media.Image;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.Button;
import android.util.Size;
import android.widget.CompoundButton;
import android.widget.Switch;

import com.google.android.material.floatingactionbutton.FloatingActionButton;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.UUID;
import java.util.Vector;


public class MainActivity extends Activity implements CameraViewFragment.CameraProcessor {  //extends AppCompatActivity
    //public class MainActivity extends Activity implements CameraViewFragment.CameraProcessor {  //extends AppCompatActivity

    private FloatingActionButton btnCapture;
    private Button btnGallery;
    private Switch swFlash;
    private Switch swDebug;
    private Switch swMask;
    private boolean flash = false;
    private boolean debug = false;

    private static Bitmap document;
    private static Bitmap document2;

    private CameraViewFragment mCamera;

    static {
        System.loadLibrary("pagescandemo");
    }

    public static Bitmap getDocumentBitmap() {
        return document;
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnCapture = (FloatingActionButton)findViewById(R.id.btnCapture);
        btnCapture.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mCamera.takePicture();
            }
        });
        btnGallery = (Button)findViewById(R.id.btnGallery);
        btnGallery.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                gallery();
            }
        });
        swFlash = (Switch)findViewById(R.id.swFlash);
        swDebug = (Switch)findViewById(R.id.swDebug);
        swMask = (Switch)findViewById(R.id.swMask);

        swFlash.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                System.out.println("FLASH CHANGED");
                mCamera.changeFlash(isChecked);
            }
        });

        swDebug.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                System.out.println("TIMER");
                nativeTimer(isChecked);
            }
        });

        swMask.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                nativeMask(isChecked);
            }
        });
        /*
        swCanny.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                nativeUseCanny(isChecked);
            }
        });

         */

        if (null == savedInstanceState) {
            mCamera = CameraViewFragment.newInstance();
            mCamera.setProcessor(this);

            getFragmentManager().beginTransaction().replace(R.id.container, mCamera)
                    .commit();
        }

        isStoragePermissionGranted();
        System.out.println(Environment.getExternalStorageDirectory());
        System.out.println(Environment.DIRECTORY_PICTURES);
        System.out.println(Environment.DIRECTORY_DCIM);

        //mCamera.mCameraManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);

        //nativeUseCanny(true);
    }

    private void gallery() {
        Intent intent = new Intent(this, GalleryActivity.class);

        startActivity(intent);
    }

    private boolean isStoragePermissionGranted() {
        if (Build.VERSION.SDK_INT >= 23) {
            if (checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED) {
                System.out.println("Permission is granted");
                return true;
            } else {

                System.out.println("Permission is revoked");
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 2);
                return false;
            }
        } else { //permission is automatically granted on sdk<23 upon installation
            System.out.println("Permission is granted");
            return true;
        }
    }


    @Override
    public void processPreview(Image image) {

        double time = System.currentTimeMillis();

        Quadrilateral q = (Quadrilateral) nativePreview(image);

        time = (System.currentTimeMillis() - time) / 1000;
        int fps = (int) (1 / time);

        Vector<Overlay> overlays = new Vector<>();
        if (q != null) {
            overlays.add(q);
            //Log.d(TAG, q.toString());
        }

        //MyPoint[] array = nativePoints();
        //Vector<Overlay> overlays = new Vector<>();
        //overlays.addAll(Arrays.asList(array));
        mCamera.setOverlays(overlays);

        //btnGallery.setText(String.format("FPS: %d, INT: %d", fps, nativeIntersections()));
        //btnGallery.setText(String.format("AVG: %d", nativeTime()));

    }

    @Override
    public void processCapture(Image image) {
        if (mCamera == null)
            return;

        if (document != null)
            document.recycle();


        if (nativeCapture(image)) {  //nativeCapture(image, swDebug.isChecked())

            Size documentSize = (Size) nativeSize();

            document = Bitmap.createBitmap(documentSize.getWidth(),
                    documentSize.getHeight(), Bitmap.Config.ARGB_8888);

            nativeBitmap(document);

/*
            Size documentSize2 = (Size) nativeSize2();

            document2 = Bitmap.createBitmap(documentSize2.getWidth(),
                    documentSize2.getHeight(), Bitmap.Config.ARGB_8888);

            nativeBitmap2(document2);

            OutputStream outputStream = null;
            try{
                //Bitmap map = Bitmap.createBitmap(finalWidth, finalHeight, Bitmap.Config.ARGB_8888);
                String path = Environment.getExternalStorageDirectory() + "/" + Environment.DIRECTORY_PICTURES + "/";
                File f = new File(path, "PageScan");
                if(!f.exists())
                    f.mkdirs();
                else
                    System.out.println("PageScan already exists");

                path = path + "PageScan/";

                File file = new File(path + UUID.randomUUID().toString() + ".jpg");

                outputStream = new FileOutputStream(file); //6000x8000????
                if (document2.compress(Bitmap.CompressFormat.JPEG, 100, outputStream)) {
                    outputStream.flush();
                    //outputStream.close();
                }

                //outputStream.write(bytes);
                System.out.println("WRITE");
                if(outputStream != null) {
                    outputStream.close();
                }

            } catch (IOException e) {
                e.printStackTrace();
            }

*/

            MyPoint[] array = nativePoints();

            Vector<Overlay> overlays = new Vector<>();
            overlays.addAll(Arrays.asList(array));

            System.out.println(String.format("Document Start (h, w): %d, %d", documentSize.getHeight(), documentSize.getWidth()));

            Intent intent = new Intent(this, DocumentActivity.class);

            startActivity(intent);

        }

        return;
    }


    public static native boolean nativeCapture(Image test);

    public static native boolean nativeBitmap(Object test);
    public static native boolean nativeBitmap2(Object test);

    public static native Object nativeSize();
    public static native Object nativeSize2();

    public static native Object nativePreview(Object image);

    public static native boolean nativeIsQuad();

    public static native void nativeTimer(boolean start);
    public static native int nativeTime();

    public static native MyPoint[] nativePoints();

    public static native int nativeIntersections();

    public static native boolean nativeUseCanny(boolean useCanny);
    public static native void nativeMask(boolean mask);


}