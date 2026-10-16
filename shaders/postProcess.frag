#version 330 core

// HDR描画結果からブルームを生成し、合成・トーンマッピング・表示用ガンマ補正を行う。

const int BRIGHT_EXTRACTION_PASS = 0;
const int BLUR_PASS = 1;
const int COMPOSITE_PASS = 2;

in vec2 textureCoordinate;

out vec4 outputColor;

// 描画側が抽出・ぼかし・合成の各パスを切り替える。
uniform int renderPass;

// sceneTextureは各パスの入力。ぼかし時には直前のブルーム結果を受け取る。
uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;

// ブルームの抽出範囲・ぼかし方向・合成強度。
uniform bool horizontalBlur;
uniform float bloomThreshold;
uniform float bloomSoftKnee;
uniform float bloomStrength;

// トーンマッピング前に適用する露出倍率。
uniform float exposure;

vec3 SanitizeHdrColor(vec3 color)
{
    // NaNや無限大がぼかしで周囲へ広がるのを防ぎ、負の色成分も除く。
    for (int channelIndex = 0; channelIndex < 3; ++channelIndex) {
        if (isnan(color[channelIndex]) || isinf(color[channelIndex])) {
            color[channelIndex] = 0.0;
        }
    }

    return max(color, vec3(0.0));
}

vec3 ExtractBrightColor(vec3 sceneColor)
{
    // 最大の色成分を明るさの指標とし、しきい値の前後を滑らかにつないで急な発光を抑える。
    float brightness = max(sceneColor.r, max(sceneColor.g, sceneColor.b));
    float safeSoftKnee = max(bloomSoftKnee, 0.0001);

    float softContribution = clamp((brightness - bloomThreshold + safeSoftKnee) / (2.0 * safeSoftKnee), 0.0, 1.0);
    softContribution *= softContribution;

    float contribution = max(brightness - bloomThreshold, softContribution * safeSoftKnee);

    // 抽出量をRGB共通の倍率に変換し、元の色の比率を保つ。
    contribution /= max(brightness, 0.0001);

    return sceneColor * max(contribution, 0.0);
}

vec3 ApplyGaussianBlur()
{
    // 中心と片側4点の重みを左右対称に使う9タップのガウシアンぼかし。
    // 描画側で横・縦パスを交互に実行し、2次元のぼかしを少ないサンプル数で近似する。
    const float SAMPLE_WEIGHTS[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

    vec2 texelSize = 1.0 / vec2(textureSize(sceneTexture, 0));
    vec2 sampleDirection = horizontalBlur ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);

    vec3 blurredColor = texture(sceneTexture, textureCoordinate).rgb * SAMPLE_WEIGHTS[0];

    for (int sampleIndex = 1; sampleIndex < 5; ++sampleIndex) {
        vec2 sampleOffset = sampleDirection * float(sampleIndex);
        blurredColor += texture(sceneTexture, textureCoordinate + sampleOffset).rgb * SAMPLE_WEIGHTS[sampleIndex];
        blurredColor += texture(sceneTexture, textureCoordinate - sampleOffset).rgb * SAMPLE_WEIGHTS[sampleIndex];
    }

    return blurredColor;
}

vec3 ApplyAcesToneMapping(vec3 hdrColor)
{
    // ACES風の近似曲線でHDRの明部を圧縮する。A・Bは分子、C〜Eは分母の係数。
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;

    return clamp((hdrColor * (A * hdrColor + B)) / (hdrColor * (C * hdrColor + D) + E), 0.0, 1.0);
}

void main()
{
    // 抽出パス: 明るい部分だけをブルーム用テクスチャへ取り出す。
    if (renderPass == BRIGHT_EXTRACTION_PASS) {
        vec3 sceneColor = SanitizeHdrColor(texture(sceneTexture, textureCoordinate).rgb);
        outputColor = vec4(ExtractBrightColor(sceneColor), 1.0);
        return;
    }

    // ぼかしパス: 指定された1方向を処理し、次のパスの入力にする。
    if (renderPass == BLUR_PASS) {
        outputColor = vec4(ApplyGaussianBlur(), 1.0);
        return;
    }

    // 合成パス: HDRのままブルームを加算し、露出・トーンマッピング・ガンマ補正の順で処理する。
    if (renderPass == COMPOSITE_PASS) {
        vec3 sceneColor = SanitizeHdrColor(texture(sceneTexture, textureCoordinate).rgb);
        vec3 bloomColor = SanitizeHdrColor(texture(bloomTexture, textureCoordinate).rgb);

        vec3 hdrColor = sceneColor + bloomColor * bloomStrength;
        vec3 toneMappedColor = ApplyAcesToneMapping(hdrColor * exposure);
        vec3 displayColor = pow(toneMappedColor, vec3(1.0 / 2.2));

        outputColor = vec4(displayColor, 1.0);
        return;
    }

    // 未定義のパス番号では黒を返す。
    outputColor = vec4(0.0, 0.0, 0.0, 1.0);
}
