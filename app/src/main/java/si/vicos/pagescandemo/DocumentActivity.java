
package si.vicos.pagescan;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.UUID;


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
                //save document
                OutputStream outputStream = null;
                try{

                    String path = Environment.getExternalStorageDirectory() + "/" + Environment.DIRECTORY_PICTURES + "/";
                    File f = new File(path, "PageScan");
                    if(!f.exists())
                        f.mkdirs();

                    path = path + "PageScan/";

                    File file = new File(path + UUID.randomUUID().toString() + ".jpg");

                    outputStream = new FileOutputStream(file);
                    if (document.compress(Bitmap.CompressFormat.JPEG, 100, outputStream)) {
                        outputStream.flush();
                    }

                    if(outputStream != null) {
                        outputStream.close();
                    }

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });

        ImageView preview = (ImageView) findViewById(R.id.document_preview);

        String imgPath = getIntent().getStringExtra("imgPath");
        if(imgPath != null)
        {
            Bitmap myBitmap = BitmapFactory.decodeFile(imgPath);
            if(myBitmap != null)
                preview.setImageBitmap(myBitmap);
        }
        else
        {
            document = MainActivity.getDocumentBitmap();
            preview.setImageBitmap(document);

        }

    }

}
