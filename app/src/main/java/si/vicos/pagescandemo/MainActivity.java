package si.vicos.pagescan;

import androidx.core.app.ActivityCompat;
import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.media.Image;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.util.Size;
import android.widget.CompoundButton;
import android.widget.Switch;

import com.google.android.material.floatingactionbutton.FloatingActionButton;

import java.util.Vector;


public class MainActivity extends Activity implements CameraViewFragment.CameraProcessor {  //extends AppCompatActivity

    private FloatingActionButton btnCapture;
    private Button btnGallery;
    private Switch swFlash;

    private static Bitmap document;

    private CameraViewFragment mCamera;

    static {
        System.loadLibrary("pagescan");
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

        swFlash.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                System.out.println("FLASH CHANGED");
                mCamera.changeFlash(isChecked);
            }
        });

        if (null == savedInstanceState) {
            mCamera = CameraViewFragment.newInstance();
            mCamera.setProcessor(this);

            getFragmentManager().beginTransaction().replace(R.id.container, mCamera)
                    .commit();
        }

        isStoragePermissionGranted();
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
        }

        mCamera.setOverlays(overlays);

    }

    @Override
    public void processCapture(Image image) {
        if (mCamera == null)
            return;

        if (document != null)
            document.recycle();


        if (nativeCapture(image)) {

            Size documentSize = (Size) nativeSize();

            document = Bitmap.createBitmap(documentSize.getWidth(),
                    documentSize.getHeight(), Bitmap.Config.ARGB_8888);

            nativeBitmap(document);

            Intent intent = new Intent(this, DocumentActivity.class);

            startActivity(intent);

        }

        return;
    }

    // native functions, implemented in pagescan.cpp

    // processes an Image and returns the outline of the document if it was found
    public static native Object nativePreview(Object image);

    // processes an Image and returns true if a document was found
    public static native boolean nativeCapture(Image test);

    // returns the size of saved document
    public static native Object nativeSize();

    // returns the saved document
    public static native boolean nativeBitmap(Object test);

}