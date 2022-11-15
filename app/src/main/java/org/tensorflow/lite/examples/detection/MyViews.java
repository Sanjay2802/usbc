package org.tensorflow.lite.examples.detection;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

public class MyViews extends View {
    ///attrs is for summa,if it is not there error will come
    public MyViews(Context context, AttributeSet attrs) {
        super(context,attrs);
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);


        int canvasW = getWidth();
        int canvasH = getHeight();
       // System.out.println("canvas height"+canvasH);

        Point centerOfCanvas = new Point(canvasW / 2, 800);
        //Point centerOfCanvas = new Point(canvasW / 2, canvasH / 2);for screen center rect box
        float rectW = 150;
        float rectH = 150;
        //box size W , H
        float left = centerOfCanvas.x - (rectW / 2);
        float top = centerOfCanvas.y - (rectH / 2);
        float right = centerOfCanvas.x + (rectW / 2);
        float bottom = centerOfCanvas.y + (rectH / 2);
        RectF rectF = new RectF(left, top, right, bottom);
        //System.out.println("Left X :"+left+" Top Y: "+top);

/////that is for center
        Paint paint = new Paint();
        Path path = new Path();



        //Rectangle
        paint.setColor(Color.parseColor("#FF0000"));
        paint.setStyle(Paint.Style.STROKE);

        canvas.drawRect(rectF, paint);

       // System.out.println(rectF);
        canvas.drawPath(path, paint);
    }}