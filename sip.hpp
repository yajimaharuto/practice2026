#include <opencv2/core.hpp>

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
void quantize(cv::Mat& blk, int* qtable) {
  const int width = blk.cols;
  const int height = blk.rows;

  const int nc = blk.channels();
  const int stride = blk.step / sizeof(float);
  float* pixel = reinterpret_cast<float*>(blk.data);
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      auto val = pixel[i * stride + j];
      auto stepsize = clamp<int>(qtable[i * stride + j]);
      // xは変数として扱われず、使用されない片方は実行時に消える
      if (X == 0)  // 量子化
        pixel[i * stride + j] = roundf(val / stepsize);
      else  // 逆量子化
        pixel[i * stride + j] = val * stepsize;
    }
  }
}