
package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BreathCircleBeautyFilter extends BaseFilter {

  private static final float BREATH_PERIOD = 3_000_000f;

  private static final float centerX = 0.5f;
  private static final float centerY = 0.5f;
  private static final float minInnerRadius = 0.0f;
  private static final float maxInnerRadius = 0.8f;
  private static final float outerRadius    = 0.8f;

  private int uInnerRadius;
  private long startTime;

  private final float deltaInnerRadius;

  public BreathCircleBeautyFilter(Context context) {
    super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.breath_circle));

    this.deltaInnerRadius = maxInnerRadius - minInnerRadius;
  }

  @Override
  protected void onInit() {
    super.onInit();

    startTime = System.currentTimeMillis();

    int uCenter      = GLES30.glGetUniformLocation(getProgramId(), "uCenter");
    uInnerRadius     = GLES30.glGetUniformLocation(getProgramId(), "uInnerRadius");
    int uOuterRadius = GLES30.glGetUniformLocation(getProgramId(), "uOuterRadius");

    setFloat(uOuterRadius, outerRadius);
    setFloatVec2(uCenter, new float[] {centerX, centerY});
  }

  @Override
  protected void onDrawArrayBefore() {
    super.onDrawArrayBefore();
    long currentTimeUs = (System.currentTimeMillis() - startTime) * 1000;
    double theta = currentTimeUs * 2 * Math.PI / BREATH_PERIOD;
    float innerRadius = minInnerRadius + deltaInnerRadius * (0.5f - 0.5f * (float) Math.cos(theta));
    setFloat(uInnerRadius, innerRadius);
  }

}
