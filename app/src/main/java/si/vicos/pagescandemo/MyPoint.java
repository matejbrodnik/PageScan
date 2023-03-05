package si.vicos.pagescandemo;

import android.graphics.Canvas;
import android.graphics.Paint;

public class MyPoint implements Overlay {

    private int x1, y1;

    public MyPoint(int x1, int y1) {
        this.x1 = x1;
        this.y1 = y1;
    }

    @Override
    public void draw(Canvas canvas, Paint paint) {

        canvas.drawCircle(this.x1, this.y1, 7, paint);

    }
}