#include <iostream>
#include <opencv2/opencv.hpp>

#include "qtable.hpp"
constexpr int FWD = 0;  // 順方向
constexpr int INV = 1;  // 逆方向

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

cv::Mat rgb2ycbcr(cv::Mat img) {
  // RGB色空間からYCbCr色空間に変換 imgはsharllowコピーなのでok
  const int width = img.cols;
  const int height = img.rows;
  const int nc = img.channels();
  const int stride = img.step;
  uint8_t* pixel = img.data;
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

  return img;
}
template <int X>
void quantize(cv::Mat& blk, const float* qtable, float Scale) {
  const int width = blk.cols;
  const int height = blk.rows;
  const int nc = blk.channels();
  const int stride = blk.step / sizeof(float);
  float* pixel = reinterpret_cast<float*>(blk.data);
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      auto val = pixel[i * stride + j];
      auto stepsize = clamp<float>(qtable[i * stride + j] * Scale);
      // xは変数として扱われず、使用されない片方は実行時に消える
      if (X == 0)  // 量子化
        pixel[i * stride + j] = roundf(val / stepsize);
      else  // 逆量子化
        pixel[i * stride + j] = val * stepsize;
    }
  }
}

int main(int argc, char* argv[]) {
  // cv::Mat image = cv::imread("./barbara.ppm", cv::IMREAD_ANYCOLOR);
  cv::Mat image = cv::imread("./../barbara.ppm", cv::IMREAD_ANYCOLOR);
  // cv::Mat image(480, 640, CV_8UC3);
  if (image.empty()) return EXIT_FAILURE;

  const int nc = image.channels();

  auto pixel = image.data;
  image = rgb2ycbcr(image);
  std::vector<cv::Mat> ycrcb;
  cv::split(image, ycrcb);  //[0] = y, [1] = Cr, [2] = Cb
  cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 0.5, 0.5);
  cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 0.5, 0.5);

  int QF;  // qを1~100に最大最小を決める必要がある
  int quality;
  if (argc < 2) {
    quality = 75;
  } else {
    quality = std::stoi(argv[1]);
  }

  if (quality <= 50)
    QF = 5000 / quality;
  else
    QF = 200 - 2 * quality;

  if (QF == 0) QF = 1;
  float Scale = QF / 100.0f;

  for (int c = 0; c < nc; ++c) {
    const int width = ycrcb[c].cols;
    const int height = ycrcb[c].rows;
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
        quantize<FWD>(blk, qmatrix[c > 0], Scale);
        // 逆量子化
        quantize<INV>(blk, qmatrix[c > 0], Scale);
        // Inverse DCT
        cv::dct(blk, blk, INV);
        blk += 128.0f;
        blk.convertTo(tmp, CV_8U);

        // エントロピー符号化
      }
    }
  }

  cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 1 / 0.5, 1 / 0.5);
  cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 1 / 0.5, 1 / 0.5);
  cv::merge(ycrcb, image);
  cv::cvtColor(image, image, cv::COLOR_YCrCb2BGR);
  cv::imshow("loaded image", image);

  cv::waitKey(0);
  cv::destroyAllWindows();
  return EXIT_SUCCESS;
}