attribute vec4 position;
attribute vec4 inputTextureCoordinate;

const int GAUSSIAN_SAMPLES = 9;

varying vec2 textureCoordinate;
varying vec2 blurCoordinates[GAUSSIAN_SAMPLES];

uniform float textureWidthOffset;
uniform float textureHeightOffset;

void main() {
    gl_Position = position;
    textureCoordinate = inputTextureCoordinate.xy;

    // Calculate the position of blur
    vec2 blurStep;
    int multiplier = 0;
    vec2 singleStepOffset = vec2(textureWidthOffset, textureHeightOffset);

    for (int i = 0; i < GAUSSIAN_SAMPLES; i++) {
        multiplier = (i - ((GAUSSIAN_SAMPLES - 1) / 2));
        // Blur in x (horizontal)
        blurStep = float(multiplier) * singleStepOffset;
        blurCoordinates[i] = inputTextureCoordinate.xy + blurStep;
    }
}