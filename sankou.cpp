#include <iostream>
#include <opencv2/opencv.hpp>

auto clamp = [](int val) {
#include "qtable.hpp"

constexpr int FWD = 0;
constexpr int INV = 1;

template <class T>
auto clamp = [](T val) {
if (val > 255) {
val = 255;
}
@@ -17,7 +23,7 @@ cv::Mat rgb2ycbcr(cv::Mat img) {
const int height = img.rows;
const int nc = img.channels();
const int stride = img.step;
  uint8_t* pixel = img.data;
  uint8_t *pixel = img.data;
for (int i = 0; i < height; ++i) {
for (int j = 0; j < width; ++j) {
int B = pixel[i * stride + j * nc + 0];
@@ -30,24 +36,39 @@ cv::Mat rgb2ycbcr(cv::Mat img) {
Cr = .5 * R - .4187 * G - .0813 * B;
Cb += 128;
Cr += 128;
      pixel[i * stride + j * nc + 0] = clamp(Y);
      pixel[i * stride + j * nc + 1] = clamp(Cr);
      pixel[i * stride + j * nc + 2] = clamp(Cb);
      pixel[i * stride + j * nc + 0] = clamp<int>(Y);
      pixel[i * stride + j * nc + 1] = clamp<int>(Cr);
      pixel[i * stride + j * nc + 2] = clamp<int>(Cb);
}
}
return img;
}

template <int X>
void quantize(cv::Mat &blk, const float *qtable, int quality = 75) {
  const int width = blk.cols;
  const int height = blk.rows;
  const int nc = blk.channels();
  const int stride = blk.step / sizeof(float);
  float *pixel = reinterpret_cast<float *>(blk.data);
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      auto val = pixel[i * stride + j];
      auto stepsize = clamp<float>(qtable[i * stride + j]);
      // 以下のif文はコンパイル時には消える
      if (X == 0)  // 量子化
        pixel[i * stride + j] = roundf(val / stepsize);
      else  // 逆量子化
        pixel[i * stride + j] = val * stepsize;
    }
  }
}

int main() {
cv::Mat image = cv::imread("./barbara.ppm", cv::IMREAD_ANYCOLOR);
if (image.empty()) return EXIT_FAILURE;

  const int width = image.cols;
  const int height = image.rows;
const int nc = image.channels();
  const int stride = image.step;
  std::cout << "width = " << width << ", ";
  std::cout << "height = " << height << std::endl;

auto pixel = image.data;
image = rgb2ycbcr(image);
@@ -56,19 +77,36 @@ int main() {
cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 0.5, 0.5);  // 444 -> 420
cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 0.5, 0.5);  // 444 -> 420

  for (int y = 0; y < height; y += 8) {
    for (int x = 0; x < width; x += 8) {
      // 起点の座標(x, y)で、8x8の領域を切り出す
      cv::Mat tmp = ycrcb[0](cv::Rect(x, y, 8, 8));
      cv::Mat blk;
      tmp.convertTo(blk, CV_32F);
      blk -= 128.0f;
      // DCT
      // 量子化
      // エントロピー符号化
  for (int c = 0; c < nc; ++c) {
    const int width = ycrcb[c].cols;
    const int height = ycrcb[c].rows;
    for (int y = 0; y < height; y += 8) {
      for (int x = 0; x < width; x += 8) {
        // 起点の座標(x, y)で、8x8の領域を切り出す
        cv::Mat tmp = ycrcb[c](cv::Rect(x, y, 8, 8));
        cv::Mat blk;
        tmp.convertTo(blk, CV_32F);
        blk -= 128.0f;
        // Forward DCT
        cv::dct(blk, blk, FWD);
        // 量子化
        quantize<FWD>(blk, qmatrix[c > 0]);
        // 逆量子化
        quantize<INV>(blk, qmatrix[c > 0]);
        // Inverse DCT
        cv::dct(blk, blk, INV);
        blk += 128.0f;
        blk.convertTo(tmp, CV_8U);
        // エントロピー符号化
      }
}
}
  cv::imshow("loaded image", ycrcb[0]);

  cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 1 / 0.5, 1 / 0.5);
  cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 1 / 0.5, 1 / 0.5);
  cv::merge(ycrcb, image);
  cv::cvtColor(image, image, cv::COLOR_YCrCb2BGR);
  cv::imshow("loaded image", image);

cv::waitKey(0);
cv::destroyAllWindows();