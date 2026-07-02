#include <chrono>
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

// argv[1]:読み込む画像 argv[2]:出力 argv[3]:quality argv[4]:YCCTYPE

int main(int argc, char* argv[]) {
  cv::Mat image = cv::imread("./../barbara.ppm", cv::IMREAD_ANYCOLOR);

  // cv::Mat image(480, 640, CV_8UC3);
  if (image.empty()) return EXIT_FAILURE;
  cv::Mat original = image.clone();  // PSNR算出のための原画像
  bitstream encbuf;

  const int nc = image.channels();

  auto pixel = image.data;
  image = rgb2ycbcr(image);
  std::vector<cv::Mat> ycrcb;
  cv::split(image, ycrcb);  //[0] = y, [1] = Cr, [2] = Cb

  int YCCtype = YUV420;

  std::ofstream outputFile("test7.2.csv");
  if (outputFile.is_open()) {
    outputFile << "bpp," << "PSNR," << YCCtype << std::endl;

  } else {
    printf("ファイルが読み込めません->終了");
    exit(EXIT_FAILURE);
  }
  for (int quality = 1; quality <= 100; quality++) {
    int dh = YCC_HV[YCCtype][0] >> 4, dV = YCC_HV[YCCtype][0] & 0x0F;

    cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 1.0f / dh, 1.0f / dV);
    cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 1.0f / dh, 1.0f / dV);
    const int width = ycrcb[0].cols;
    const int height = ycrcb[0].rows;

    int QF;  // qを1~100に最大最小を決める必要がある

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
        if (qtable[0][i] == 0) {
          qtable[0][i] = 1;
        }
        if (qtable[1][i]) {
          qtable[1][i] = 1;
        }
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

    // std::cout << "codestream size = " << length << ". ";
    double bitrate = static_cast<double>(length) * 8.0 / (width * height);

    cv::resize(ycrcb[1], ycrcb[1], cv::Size(), dh, dV);
    cv::resize(ycrcb[2], ycrcb[2], cv::Size(), dh, dV);
    cv::merge(ycrcb, image);
    cv::cvtColor(image, image, cv::COLOR_YCrCb2BGR);
    double mse = 0.0;
    for (int i = 0; i < height; ++i) {
      for (int j = 0; j < width; ++j) {
        for (int c = 0; c < nc; ++c) {
          int v0 = original.data[i * original.step + j * nc];
          int v1 = image.data[i * image.step + j * nc + c];
          mse += (v0 - v1) * (v0 - v1);
        }
      }
    }
    mse /= (width * height * nc);
    double psnr = 10 * log10(255 * 255 / mse);
    // std::cout << psnr << std::endl;
    // cv::imshow("loaded image", image);
    outputFile << bitrate << "," << psnr << std::endl;
    // cv::waitKey(0);
    // cv::destroyAllWindows();
  }
  outputFile.close();
  return EXIT_SUCCESS;
}
// ファイルサイズからbit rate(bit/pixel)への変換
// bpp(bit per pixel) = (filesize*8)/(width* height)

// RD曲線  横軸にbitrate縦軸に画質(様々な指標がある)をとること
// PSNR　

// 課題1：横軸bpp、縦軸psnr[dB]のRD曲線をbarbara.ppm+a(後で追加する画像)それぞれについてグラフにする
// 課題2：以下のJPEGエンコード処理の中でどの処理が最も処理時間を食っているかを調査
// ビットレートによって変化

// RGB-> YCbCr色空間変換
// DCT
// 量子化
// エントロピー符号化

// tic = chrono::hidhresoluton_clock::now();開始
// toc = chrono::hidhresoluton_clock::now();終了
// size_tduration = std::chrono
// ::duration_cast<std::chrono::microsecond>(tic-toc).count();
