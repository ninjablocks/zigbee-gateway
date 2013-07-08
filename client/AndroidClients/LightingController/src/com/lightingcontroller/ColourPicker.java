/**************************************************************************************************
  Filename:       ColourPicker.java
  Revised:        $$
  Revision:       $$

  Description:    Color Picker UI

  Copyright (C) 2007 The Android Open Source Project

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
  
        http://www.apache.org/licenses/LICENSE-2.0
	
  Unless required by applicable law or agreed to in writing, software	
  distributed under the License is distributed on an "AS IS" BASIS,	
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.	
  See the License for the specific language governing permissions and	
  limitations under the License.	

  Copyright (C) {2012} Texas Instruments Incorporated - http://www.ti.com/


  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

     Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

     Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the
     distribution.

     Neither the name of Texas Instruments Incorporated nor the names of
     its contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
**************************************************************************************************/

package com.lightingcontroller;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ComposeShader;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.RadialGradient;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.SweepGradient;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

public class ColourPicker extends View 
{    
  private static final float PI = 3.1415926f;

  private int[] mCoord;
  private float[] mHSV;
  
  private zbLighitngMain ourContext;
  
  private static int CENTER_X;
  private static int CENTER_Y;
  private static int HUE_RADIUS;
  private static int INNER_RADIUS;
  private static int PALETTE_RADIUS;

  private static int SAT_RADIUS;
  
  public boolean colourKnown = false;
    
  int[] mSpectrumColorsRev = new int[] {
    0xFFFF0000, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
    0xFF00FF00, 0xFFFFFF00, 0xFFFF0000,
  };
  
  Paint mOvalHue;
  Paint mOvalHueInner;
  
  Paint mOvalSat;
  RectF mRectSat;
  Paint mArcSat;
  Paint mPaintSatTextRect;
  Paint mPaintSatText;
    
  Paint mPosMarker;
  RectF posMarkerRect1;
  RectF posMarkerRect2;  
  
  Shader shaderA;
  Shader shaderB;   
  Shader shaderHue;
    
  Shader shaderSat;

  
  public ColourPicker(Context context, AttributeSet attr) {
    super(context, attr);
    
    ourContext = (zbLighitngMain)context;
  
    DisplayMetrics metrics = new DisplayMetrics();
    WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
    wm.getDefaultDisplay().getMetrics(metrics);   
    
    double scalefactor;
    
    if(metrics.widthPixels < metrics.heightPixels)
    {
      scalefactor = 2.5;
    }
    else
    {
      scalefactor = 5.5;
    }
  
    CENTER_X = (int) ( 1 * (metrics.widthPixels/scalefactor));// (metrics.heightPixels / 2);
    CENTER_Y =  (int) ( 1 * (metrics.widthPixels/scalefactor));// (metrics.heightPixels / 2);
    HUE_RADIUS = (int) ( 1 * (metrics.widthPixels/scalefactor));//(metrics.heightPixels / 6) - (metrics.heightPixels / 45); //110;
    INNER_RADIUS =  (int) ( 0.63 * (metrics.widthPixels/scalefactor));// (metrics.heightPixels / 10.5); //70;
    PALETTE_RADIUS = (int) ( 1 * (metrics.widthPixels/scalefactor));//(metrics.heightPixels / 6) - (metrics.heightPixels / 45); //110;;  
    SAT_RADIUS = (int) ( 0.60 * (metrics.widthPixels/scalefactor));//(metrics.heightPixels / 10.5) -5; //65;*/
    
    mOvalHue = new Paint(Paint.ANTI_ALIAS_FLAG);
    mOvalHueInner = new Paint(Paint.ANTI_ALIAS_FLAG);
  
    mOvalSat = new Paint(Paint.ANTI_ALIAS_FLAG);
    mRectSat = new RectF( -SAT_RADIUS, -SAT_RADIUS,  SAT_RADIUS, SAT_RADIUS);
    mArcSat = new Paint();
      
    mPosMarker = new Paint(Paint.ANTI_ALIAS_FLAG);
    
    shaderA = new SweepGradient(0, 0, mSpectrumColorsRev, null);
    shaderB = new RadialGradient(CENTER_X, CENTER_Y, HUE_RADIUS, 0xFFFFFFFF, 0xFF000000, Shader.TileMode.CLAMP);   
    shaderHue = new ComposeShader(shaderA, shaderB, PorterDuff.Mode.SCREEN);
      
    shaderSat = new RadialGradient(CENTER_X, CENTER_Y, SAT_RADIUS, 0xFF888888, 0xFFFFFFFF, Shader.TileMode.CLAMP);   
    
    // InitialisePaints Paints
    mOvalHue.setShader(shaderHue);
    mOvalHue.setStyle(Paint.Style.FILL);
    mOvalHue.setDither(true);            
    
    mOvalSat.setShader(shaderSat);
    mOvalSat.setStyle(Paint.Style.FILL);
    mOvalSat.setDither(true);
    mOvalSat.setColor(0xFFFFFFFF);
    
    mArcSat.setAntiAlias(true);            
    mArcSat.setStyle(Paint.Style.FILL);            
    mArcSat.setColor(0xFFFFFFFF);                        
        
    mPosMarker.setStyle(Paint.Style.STROKE);
    mPosMarker.setStrokeWidth(2);
  
   	mPaintSatTextRect = new Paint();
    mPaintSatTextRect.setAntiAlias(true);            
    mPaintSatTextRect.setStyle(Paint.Style.FILL);            
    mPaintSatTextRect.setColor(0XFF000000); 
  
    mPaintSatText = new Paint();
    mPaintSatText.setAntiAlias(true);            
    mPaintSatText.setStyle(Paint.Style.FILL);            
    mPaintSatText.setColor(0xFFFFFFFF);  
    mPaintSatText.setTextSize(25);
        
    mCoord = new int[2];
    mHSV = new float[3];
    mHSV[1] = 1;    
    
    posMarkerRect1 = new RectF(mCoord[0] - 5, mCoord[1] - 5, mCoord[0] + 5, mCoord[1] + 5);
    posMarkerRect2 = new RectF(mCoord[0] - 3, mCoord[1] - 3, mCoord[0] + 3, mCoord[1] + 3);
  }


  @Override 
  protected void onDraw(Canvas canvas) {
            
    canvas.translate(CENTER_X, CENTER_Y);
      
    canvas.drawCircle(0, 0, HUE_RADIUS, mOvalHue);
    canvas.drawCircle(0, 0, INNER_RADIUS, mOvalHueInner);
      
    //Sat up
    canvas.drawArc(mRectSat, (float)182, (float)176, true, mOvalSat);         
    canvas.drawArc(mRectSat, (float)182, (float)176, true, mArcSat);
    
    //Sat down
    canvas.drawArc(mRectSat, (float)2, (float)176, true, mOvalSat);
    canvas.drawArc(mRectSat, (float)2, (float)176, true, mArcSat);        
    
    canvas.drawRect(-(SAT_RADIUS-10), 15, (SAT_RADIUS-10), -15, mPaintSatTextRect);                     
      
    mPaintSatText.setColor(0xFFFFFFFF);  
    mPaintSatText.setTextSize(25);
    String satStr = "Saturation"; 
    canvas.drawText(satStr, -60, 8, mPaintSatText);           
      
    mPaintSatText.setTextSize(35);
    mPaintSatText.setColor(0xFF000000);
    satStr = "+"; 
    canvas.drawText(satStr, -8, -30, mPaintSatText);  
    satStr = "-"; 
    canvas.drawText(satStr, -8, 50, mPaintSatText); 
    
    if (colourKnown)
    {
      posMarkerRect1.set(mCoord[0] - 5, mCoord[1] - 5, mCoord[0] + 5, mCoord[1] + 5);
      posMarkerRect2.set(mCoord[0] - 3, mCoord[1] - 3, mCoord[0] + 3, mCoord[1] + 3);
        
      mPosMarker.setColor(Color.BLACK);      
      canvas.drawOval(posMarkerRect1, mPosMarker);
      mPosMarker.setColor(Color.WHITE);
      canvas.drawOval(posMarkerRect2, mPosMarker);
    }       
  }
  
  // Currently Fixed size
  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    setMeasuredDimension(CENTER_X*2, CENTER_Y*2);
  }
  
 
  public void drawPortraite()
  {
    invalidate();  
  }
  
  
  // Weighted average between points
  private int ave(int s, int d, float p) {
    return s + java.lang.Math.round(p * (d - s));
  }
  
  // Interpolate colour value between points
  private int interpColor(int colors[], float unit) {
    if (unit <= 0) {
        return colors[0];
    }
    if (unit >= 1) {
        return colors[colors.length - 1];
    }
    
    float p = unit * (colors.length - 1);
    int i = (int)p;
    p -= i;

    // now p is just the fractional part [0...1] and i is the index
    int c0 = colors[i];
    int c1 = colors[i+1];
    int a = ave(Color.alpha(c0), Color.alpha(c1), p);
    int r = ave(Color.red(c0), Color.red(c1), p);
    int g = ave(Color.green(c0), Color.green(c1), p);
    int b = ave(Color.blue(c0), Color.blue(c1), p);
    System.out.println("the vaient is"+b);
    return Color.argb(a, r, g, b);
  }

private int round(double x) {
  return (int)Math.round(x);
}
  
public void upDateColorPreivew(byte hue, byte sat)
{ 
  //update hue preview
  float unit = (float) hue / 255;
  if (unit < 0) {
    unit += 1;
  }
  
  unit = 1 - unit;
  
  int c = interpColor(mSpectrumColorsRev, unit );
  mArcSat.setColor(c);
  
  //update sat preview
  mArcSat.setAlpha(sat);    
  
  colourKnown = true;
}

  @Override
  public boolean onTouchEvent(MotionEvent event) {
      
    if (!isEnabled())
        return false;

    float x = event.getX() - CENTER_X;
    float y = event.getY() - CENTER_Y;
                
    float angle = (float)java.lang.Math.atan2(y, x);
    // need to turn angle [-PI ... PI] into unit [0....1]
    float unit = angle/(2*PI);
                
    if (unit < 0) {
      unit += 1;
    }           
                
    //Pin the radius
    float radius = (float)java.lang.Math.sqrt(x * x + y * y);
    if (radius > PALETTE_RADIUS)
      radius = PALETTE_RADIUS;
                
    if( radius < INNER_RADIUS )
    {           
      //User adjusted saturation
      if(angle < 0)
      {         
        //+ Sat 
        if((mHSV[1]+ 0.10) <= 1)
        {       
          mHSV[1] += 0.10;
        }       
        else    
        {       
          mHSV[1] = 1;
        }       
      }         
      else      
      {         
        //- Sat 
        if((mHSV[1] - 0.10) >= 0)
        {       
          mHSV[1] -=0.10;
        }       
        else    
        {       
          mHSV[1] = 0;
        }       
      }         
                
      byte hue = (byte) ((mHSV[0]/360)*255);
      byte sat = (byte) (mHSV[1]*254);
      ourContext.sendHueSatChange(hue, sat);
      //update preview
      mArcSat.setAlpha((byte) (mHSV[1]*254));
    }           
    else        
    {           
      //User adjusted hue   
      mCoord[0] = round(Math.cos(angle) * (HUE_RADIUS - (HUE_RADIUS-INNER_RADIUS)/2));
      mCoord[1] = round(Math.sin(angle) * (HUE_RADIUS - (HUE_RADIUS-INNER_RADIUS)/2));
                
      int c = interpColor(mSpectrumColorsRev, unit);
      float[] hsv = new float[3];
      Color.colorToHSV(c, hsv);
      mHSV[0] = hsv[0];
                
      colourKnown = true;
                
      // Update color
      byte hue = (byte) ((mHSV[0]/360)*255);
      byte sat = (byte) (mHSV[1]*254);
      ourContext.sendHueSatChange(hue, sat);
                
      //update preview
      mArcSat.setColor(c);
      mArcSat.setAlpha((int)(mHSV[1]*255));
    }           
                
    invalidate();
                
    return true;
  }
  
}

