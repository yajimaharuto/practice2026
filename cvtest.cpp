#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
  cv::Mat image = cv::imread("./barbara.ppm", cv::IMREAD_ANYCOLOR);
  // cv::Mat image(480, 640, CV_8UC3);
  if (image.empty()) return EXIT_FAILURE;

  const int width = image.cols;
  const int height = image.rows;
  const int nc = image.channels();
  const int stride = image.step;
  std::cout << "width = " << image.cols << ", ";
  std::cout << "height = " << image.rows << std::endl;

  auto pixel = image.data;
  auto clamp = [](int val) {
    if (val > 255) {
      val = 255;
    }
    if (val < 0) {
      val = 0;
    }
    return val;
  };

  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      int B = pixel[i * stride + j * nc + 0];
      int G = pixel[i * stride + j * nc + 1];
      int R = pixel[i * stride + j * nc + 2];
      pixel[i * stride + j * nc + 0] = R;
      pixel[i * stride + j * nc + 2] = B;
      B = clamp(B * 4);
      G = clamp(G * 4);
      R = clamp(R * 4);
    }
  }
  cv::imshow("loaded image", image);

  cv::waitKey(0);
  cv::destroyAllwindows();
  return EXIT_SUCCESS;
}