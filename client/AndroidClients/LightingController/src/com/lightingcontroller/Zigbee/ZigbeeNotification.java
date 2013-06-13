/**************************************************************************************************
  Filename:       ZigBeeNotification.java
  Revised:        $$
  Revision:       $$

  Description:    ZigBee Notification Class

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

package com.lightingcontroller.Zigbee;

import java.util.ArrayList;

import com.lightingcontroller.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.widget.Toast;

public class ZigbeeNotification {

	static Activity acty;
	static Handler handle = new Handler(); 		
	static boolean alive;
	static PopupWindow popwindow = null;
	
	static ArrayList<String> notices = new ArrayList<String>();
	
	static int timeout = 5000;
	
	public static void init(Activity a, int time_out)
	{
		init(a);
		timeout = time_out;
	}
	
	public static void init(Activity a)
	{
		acty = a;
		if (notices.size() > 0)
		{
			for (int i = 0 ; i < notices.size() ; i++)
			{
				showNotificationOnScreen(notices.get(i));
			}
			for (int i = 0 ; i < notices.size() ; i++)
			{
				notices.remove(i);
			}			
		}
	}
	
	public static void closing()
	{
		if (popwindow != null)
		{
			popwindow.dismiss();
			handle.removeCallbacks(closePopupTask);
		}
		popwindow = null;
		acty = null;
	}
	
	public static void showNotification(String text)
	{
		if (acty == null)
		{
			notices.add(text);
		}
		else
		{
			showNotificationOnScreen(text);
		}
	}
	
	public static void showNotificationOnScreen(final String text)
	{
		final TextView text1 = new TextView(acty);
		text1.setText(text);
		text1.setTextColor(Color.WHITE);
		text1.setTextSize(18);
		text1.setGravity(Gravity.CENTER);
				
		if (popwindow == null)
		{
			LayoutInflater inflater = (LayoutInflater) acty.getLayoutInflater();
			View layout = inflater.inflate(R.layout.popup,
			                               (ViewGroup) acty.findViewById(R.id.toast_layout_root));	
	
			((LinearLayout) layout.findViewById(R.id.popup_text_box)).addView(text1);

			popwindow = new PopupWindow(layout,400,100,false); 
			popwindow.setAnimationStyle(R.style.Animation_Popup);
			popwindow.setBackgroundDrawable(new BitmapDrawable());
			popwindow.setOutsideTouchable(false);
			popwindow.setTouchable(true);
			popwindow.setTouchInterceptor(new OnTouchListener() {
			public boolean onTouch(View v, MotionEvent event) {
				handle.removeCallbacks(closePopupTask);			
				handle.post(closePopupTask);
				return false;
				}
			});
			try{
				acty.runOnUiThread (new Runnable(){
					public void run() {
//						if (acty!=null && acty.findViewById(R.id.MainMenu_lay)!=null)
//							popwindow.showAtLocation(acty.findViewById(R.id.MainMenu_lay), Gravity.CENTER_HORIZONTAL|Gravity.BOTTOM, 0, 0);
//						else
//							notices.add(text);
					}
				});
			} catch (Exception e)
			{
				// Dagnabbit.
				popwindow = null;
			}
		}
		else
		{
			
			acty.runOnUiThread (new Runnable(){
				public void run() {
					LinearLayout l = (LinearLayout)popwindow.getContentView().findViewById(R.id.popup_text_box);
					if (l.getChildCount()>2)
						l.removeViewAt(0);
					l.addView(text1);
					popwindow.update();
				}
			});
		}
				
		handle.removeCallbacks(closePopupTask);
		handle.postDelayed(closePopupTask, timeout);
		//SoundManager.playSound(SoundManager.NOTIFY, 1);
	}
	
	
	private static Runnable closePopupTask = new Runnable() {
		   public void run() {
			   if (popwindow!=null)
			   {
				   popwindow.dismiss();
				   popwindow = null;
			   }
		   }
		};
	
	
}
