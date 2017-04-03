/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modified by Opera Software ASA
 * @copied-from chromium/src/third_party/android_swipe_refresh/java/src/org/chromium/third_party/android/swiperefresh/CircleImageView.java
 */

package org.chromium.third_party.android.swiperefresh;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.OvalShape;
import android.view.animation.Animation;
import android.widget.ImageView;

/**
 * Private class created to work around issues with AnimationListeners being
 * called before the animation is actually complete and support shadows on older
 * platforms.
 *
 * @hide
 */
class CircleImageView extends ImageView {

    private static final float STROKE_RADIUS = 1.5f;

    private Animation.AnimationListener mListener;

    @SuppressWarnings("deprecation")
    public CircleImageView(Context context, int color, final float radius) {
        super(context);
        final float density = getContext().getResources().getDisplayMetrics().density;

        final int strokeDiameter = (int) (density * STROKE_RADIUS);
        final int diameter = (int) (density * radius * 2);
        ShapeDrawable circle = new ShapeDrawable(new OvalBorderShape(diameter, strokeDiameter));

        circle.getPaint().setColor(color);
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.JELLY_BEAN) {
            setBackgroundDrawable(circle);
        } else {
            setBackground(circle);
        }
    }

    public void setAnimationListener(Animation.AnimationListener listener) {
        mListener = listener;
    }

    @Override
    public void onAnimationStart() {
        super.onAnimationStart();
        if (mListener != null) {
            mListener.onAnimationStart(getAnimation());
        }
    }

    @Override
    public void onAnimationEnd() {
        super.onAnimationEnd();
        if (mListener != null) {
            mListener.onAnimationEnd(getAnimation());
        }
    }

    /**
     * Update the background color of the circle image view.
     *
     * @param colorRes Id of a color resource.
     */
    public void setBackgroundColorRes(int colorRes) {
        setBackgroundColor(getContext().getResources().getColor(colorRes));
    }

    @Override
    public void setBackgroundColor(int color) {
        if (getBackground() instanceof ShapeDrawable) {
            ((ShapeDrawable) getBackground()).getPaint().setColor(color);
        }
    }

    public void setStrokeColor(int color) {
        ((OvalBorderShape)((ShapeDrawable) getBackground()).getShape()).setStrokeColor(color);
    }

    private class OvalBorderShape extends OvalShape {
        private final int mCircleDiameter;
        private final int mStrokeDiameter;
        private Paint mStrokePaint;

        public OvalBorderShape(int circleDiameter, int strokeWidth) {
            mCircleDiameter = circleDiameter - strokeWidth;
            mStrokeDiameter = circleDiameter;

            mStrokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            mStrokePaint.setColor(getResources().getColor(android.R.color.black));
        }

        public void setStrokeColor(int color) {
            mStrokePaint.setColor(color);
        }

        @Override
        public void draw(Canvas canvas, Paint paint) {
            final int viewHeight = CircleImageView.this.getHeight();
            final int viewWidth = CircleImageView.this.getWidth();

            canvas.drawCircle(viewWidth / 2f, viewHeight / 2f, mStrokeDiameter / 2f, mStrokePaint);
            canvas.drawCircle(viewWidth / 2f, viewHeight / 2f, mCircleDiameter / 2f, paint);
        }
    }
}
