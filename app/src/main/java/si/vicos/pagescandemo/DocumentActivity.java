
package si.vicos.pagescandemo;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
//import android.support.design.widget.FloatingActionButton;
//import android.support.design.widget.Snackbar;
//import android.support.v7.app.AppCompatActivity;
//import android.support.v7.widget.Toolbar;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;
import androidx.appcompat.app.AppCompatActivity;

//import com.aspose.words.Document;
import com.google.android.material.appbar.AppBarLayout;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;
import com.squareup.picasso.Picasso;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.UUID;
//import com.lowagie.text.*;
//import com.lowagie.text.pdf.*;
//import com.aspose.words.*;


public class DocumentActivity extends AppCompatActivity {

    private Bitmap document;

    private Toolbar toolbar;
    private TextView textView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        System.out.println("Document Create");

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_document);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
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
                    if (document.compress(Bitmap.CompressFormat.JPEG, 100, outputStream)) {
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
            }
        });

        ImageView preview = (ImageView) findViewById(R.id.document_preview);

        //------
        String imgPath = getIntent().getStringExtra("imgPath");
        if(imgPath != null)
        {
            Bitmap myBitmap = BitmapFactory.decodeFile(imgPath);
            if(myBitmap != null)
                preview.setImageBitmap(myBitmap);
        }
        //------
        else
        {
            document = MainActivity.getDocumentBitmap();
            preview.setImageBitmap(document);

        }

        /*
        String path = Environment.getExternalStorageDirectory() + "/" + Environment.DIRECTORY_PICTURES + "/PageScan/";

        Document doc = null;
        try {
            doc = new Document();
        } catch (Exception e) {
            e.printStackTrace();
        }

        DocumentBuilder builder = new DocumentBuilder(doc);

        try {
            builder.insertImage(path + "slika.jpg");
        } catch (Exception e) {
            e.printStackTrace();
        }

        try {
            doc.save(path + "Output.pdf");
        } catch (Exception e) {
            e.printStackTrace();
        }
*/


        textView = findViewById(R.id.textView);
        toolbar = findViewById(R.id.toolbar);
        String s = "NO";
        if(MainActivity.nativeIsQuad())
            s = "YES";
        textView.setText(s);

        System.out.println("Document Finish");
    }

}
