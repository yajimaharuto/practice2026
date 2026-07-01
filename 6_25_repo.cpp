#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "bitstream.hpp"
#include "coding.hpp"
#include "jpgheaders.hpp"
#include "qtable.hpp"
#include "ycctype.hpp"

constexpr int Luma = 0;
constexpr int Chroma = 1;

int main() {
  cv::Mat image = cv::imread("./../barbara.ppm", cv::IMREAD_ANYCOLOR);
  // cv::Mat image(480, 640, CV_8UC3);
  if (image.empty()) return EXIT_FAILURE;
  bitstream encbuf;

  const int nc = image.channels();

  auto pixel = image.data;
  image = rgb2ycbcr(image);
  std::vector<cv::Mat> ycrcb;
  cv::split(image, ycrcb);  //[0] = y, [1] = Cr, [2] = Cb

  int YCCtype = YUV410;

  int dh = YCC_HV[YCCtype][0] >> 4, dV = YCC_HV[YCCtype][0] & 0x0F;

  cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 1.0f / dh, 1.0f / dV);
  cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 1.0f / dh, 1.0f / dV);
  const int width = ycrcb[0].cols;
  const int height = ycrcb[0].rows;

  std::ofstream outputFile("test6_25.csv");
  if (outputFile.is_open()) {
    outputFile << "quality," << "bitsize," << YCCtype << std::endl;

  } else {
    printf("ファイルが読み込めません->終了");
    exit(EXIT_FAILURE);
  }
  for (int i = 1; i <= 100; i++) {
    int QF;  // qを1~100に最大最小を決める必要がある
    int quality = i;

    if (quality <= 50)
      QF = 5000 / quality;
    else
      QF = 200 - 2 * quality;

    if (QF == 0) QF = 1;
    float Scale = QF / 100.0f;

    int qtable[2][64];
    for (int i = 0; i < 64; ++i) {
      if (QF != 1) {
        qtable[0][i] = static_cast<int>(clamp<float>(qmatrix[0][i] * Scale));
        qtable[1][i] = static_cast<int>(clamp<float>(qmatrix[1][i] * Scale));
      } else {
        qtable[0][i] = qtable[1][i] = 1;
      }
    }

    // MCU の処理

    // prev_dc にはひとつ前のdc成分をいれる
    int prev_dc[3] = {0};
    for (int y = 0, cy = 0; y < height; y += 8 * dV, cy += 8) {
      for (int x = 0, cx = 0; x < width; x += 8 * dh, cx += 8) {
        for (int i = 0; i < dV; ++i) {
          for (int j = 0; j < dh; ++j) {
            // 起点の座標(x, y)で、8*8の領域を切り出す
            cv::Mat tmpY = ycrcb[0](cv::Rect(x + j * 8, y + i * 8, 8, 8));
            blkproc(tmpY, qtable[Luma], prev_dc[0], Luma, encbuf);
          }
        }
        // 起点の座標(x, y)で、8*8の領域を切り出す
        cv::Mat tmpCr = ycrcb[2](cv::Rect(cx, cy, 8, 8));  // Cb
        blkproc(tmpCr, qtable[Chroma], prev_dc[1], Chroma, encbuf);
        cv::Mat tmpCb = ycrcb[1](cv::Rect(cx, cy, 8, 8));  // Cr
        blkproc(tmpCb, qtable[Chroma], prev_dc[2], Chroma, encbuf);
      }
    }
    size_t length = encbuf.finalize();

    // std::cout << "codestream size = " << length << std::endl;

    cv::resize(ycrcb[1], ycrcb[1], cv::Size(), dh, dV);
    cv::resize(ycrcb[2], ycrcb[2], cv::Size(), dh, dV);
    cv::merge(ycrcb, image);
    cv::cvtColor(image, image, cv::COLOR_YCrCb2BGR);

    outputFile << quality << "," << length << std::endl;
    // cv::imshow("loaded image", image);

    // cv::waitKey(0);
    // cv::destroyAllWindows();
  }
  outputFile.close();
  return EXIT_SUCCESS;
}
// qualityを変化させ、quality値とファイルサイズを記録し...

// ビットレートは1画素当たりに何ビット使用しているかを表す値、
// 先ほどのファイルサイズと原画像のサイズから求められる関係を表す何かを作る