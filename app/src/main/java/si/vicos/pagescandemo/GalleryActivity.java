package si.vicos.pagescan;

import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.io.File;
import java.util.ArrayList;

import static android.Manifest.permission.READ_EXTERNAL_STORAGE;

public class GalleryActivity extends AppCompatActivity {

    // on below line we are creating variables for
    // our array list, recycler view and adapter class.
    private static final int PERMISSION_REQUEST_CODE = 200;
    private ArrayList<String> imagePaths;
    private RecyclerView imagesRV;
    private RecyclerViewAdapter imageRVAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_gallery);

        imagePaths = new ArrayList<>();
        imagesRV = findViewById(R.id.idRVImages);

        prepareRecyclerView();
        requestPermissions();

    }

    private boolean checkPermission() {
        // in this method we are checking if the permissions are granted or not and returning the result.
        int result = ContextCompat.checkSelfPermission(getApplicationContext(), READ_EXTERNAL_STORAGE);
        return result == PackageManager.PERMISSION_GRANTED;
    }

    private void requestPermissions() {
        if (checkPermission()) {
            // if the permissions are already granted we are calling
            // a method to get all images from our external storage.
            Toast.makeText(this, "Permissions granted..", Toast.LENGTH_SHORT).show();
            getImagePath();
        } else {
            // if the permissions are not granted we are
            // calling a method to request permissions.
            requestPermission();
        }
    }

    private void requestPermission() {
        //on below line we are requesting the rea external storage permissions.
        ActivityCompat.requestPermissions(this, new String[]{READ_EXTERNAL_STORAGE}, PERMISSION_REQUEST_CODE);
    }

    private void prepareRecyclerView() {
        // in this method we are preparing our recycler view.
        // on below line we are initializing our adapter class.
        imageRVAdapter = new RecyclerViewAdapter(GalleryActivity.this, imagePaths);

        // on below line we are creating a new grid layout manager.
        GridLayoutManager manager = new GridLayoutManager(GalleryActivity.this, 3);

        // on below line we are setting layout
        // manager and adapter to our recycler view.
        imagesRV.setLayoutManager(manager);
        imagesRV.setAdapter(imageRVAdapter);
    }

    private void getImagePath() {
        // in this method we are adding all our image paths
        // in our arraylist which we have created.
        // on below line we are checking if the device is having an sd card or not.
        boolean isSDPresent = android.os.Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);

        if (isSDPresent) {

            String path = Environment.getExternalStorageDirectory() + "/" + Environment.DIRECTORY_PICTURES + "/PageScan/";
            File directory = new File(path);
            File[] files = directory.listFiles();
            Log.d("Files", "Size: "+ files.length);
            for (int i = 0; i < files.length; i++)
            {
                Log.d("Files", "FileName:" + files[i].getName());
                imagePaths.add(path + files[i].getName());
            }
            imageRVAdapter.notifyDataSetChanged();

        }
    }

}