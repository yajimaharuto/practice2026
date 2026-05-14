#include <iostream>
#include <opencv2/opencv.hpp>

#include "qtable.hpp"

constexpr int FWD = 0;  // 順方向
constexpr int INV = 0;  // 逆方向

template <class T>
auto clamp = [](T val) {
  if (val > 255) {
    val = 255;
  }
  if (val < 0) {
    val = 0;
  }
  return val;
};

cv::Mat rgb2ycbcr(cv::Mat image) {
  // RGB色空間からYCbCr色空間に変換 imgはsharllowコピーなのでok
  const int width = image.cols;
  const int height = image.rows;
  const int nc = image.channels();
  const int stride = image.step;
  uint8_t* pixel = image.data;
  for (int c = 0; c < nc; ++c) {
    for (int i = 0; i < height; ++i) {
      for (int j = 0; j < width; ++j) {
        int B = pixel[i * stride + j * nc + 0];
        int G = pixel[i * stride + j * nc + 1];
        int R = pixel[i * stride + j * nc + 2];

        int Y, Cb, Cr;
        Y = 0.299 * R + 0.587 * G + 0.114 * B;
        Cb = -0.1687 * R + -0.3313 * G + 0.5 * B;
        Cr = 0.5 * R + -0.4187 * G + -0.0813 * B;
        Cb += 128;
        Cr += 128;

        pixel[i * stride + j * nc + 0] = clamp<int>(Y);
        pixel[i * stride + j * nc + 1] = clamp<int>(Cr);
        pixel[i * stride + j * nc + 2] = clamp<int>(Cb);
      }
    }
  }
  return image;
}

template <int x>
void quantize(cv::Mat& blk, const float, *qtale, int Quality = 75) {
  const int width = blk.cols;
  const int height = blk.rows;
  const int nc = blk.channels();
  const int stride = blk.step / sizeof(float);
  float* pixel = reinterpret_cast<float*>(blk.data);
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < weidth; ++j) {
      auto val = pixel[i * stride + j];
      auto stepsize = clamp<float> qtable[i * stride + j];
      // xは変数として扱われず、使用されない片方は実行時に消える
      if (x == 0)  // 量子化
        pixel[i * stride + j] = roundf(val / stepsize);
      else  // 逆量子化
        pixel[i * stride + j] = val * stepsize;
    }
  }
}

int main() {
  // cv::Mat image = cv::imread("./barbara.ppm", cv::IMREAD_ANYCOLOR);
  cv::Mat image =
      cv::imread("H:/practice2026/barbara.ppm", cv::IMREAD_ANYCOLOR);
  // cv::Mat image(480, 640, CV_8UC3);
  if (image.empty()) return EXIT_FAILURE;

  const int width = image.cols;
  const int height = image.rows;
  const int nc = image.channels();
  const int stride = image.step;
  std::cout << "width = " << image.cols << ", ";
  std::cout << "height = " << image.rows << std::endl;

  auto pixel = image.data;
  image = rgb2ycbcr(image);
  std::vector<cv::Mat> ycrcb;
  cv::split(image, ycrcb);  //[0] = y, [1] = Cr, [2] = Cb
  cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 0.5, 0.5);
  cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 0.5, 0.5);

  for (int y = 0; y < height; y += 8) {
    for (int x = 0; x < width; x += 8) {
      // 起点の座標(x, y)で、8*8の領域を切り出す
      cv::Mat tmp = ycrcb[0](cv::Rect(x, y, 8, 8));
      cv::Mat blk;
      tmp.convertTo(blk, CV_32F);
      blk -= 128.0f;
      // DCT
      cv::dct(blk, blk, FWD);
      // 量子化
      quantize<FWD>(blk, qmatrix[0]);
      // 逆量子化
      quantize<INV>(blk, qmatrix[0]);
      // Inverse DCT
      cv::dct(blk, blk, 1);

      blk.convertTo(tmp, CV_8U);

      // エントロピー符号化
    }
  }

  int x, y;
  x = 447;
  y = 68;
  cv::imshow("loaded image", ycrcb[0](cv::Rect(x, y, 100, 100)));

  // cv::cvtColor(image, image, cv::COLOR_BGR2YCrCb)
  cv::imshow("loaded image", image);

  cv::waitKey(0);
  cv::destroyAllWindows();
  return EXIT_SUCCESS;
}