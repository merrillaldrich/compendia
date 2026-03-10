#include "perceptualhasher.h"

#include <cmath>
#include <algorithm>

/*! \brief Computes a 64-bit perceptual hash for the given image.
 *
 * Scales the image to 32×32 greyscale, applies a 2-D DCT, selects the
 * top-left 8×8 low-frequency block, and compares each coefficient against
 * the block median to produce a 64-bit hash.
 *
 * \param image Source image (any size or format).
 * \return 64-bit pHash value, or 0 if the image is null.
 */
quint64 PerceptualHasher::computeHash(const QImage &image)
{
    if (image.isNull())
        return 0;

    const int N = 32;
    const int K = 8;

    QImage small = image.scaled(N, N, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                       .convertToFormat(QImage::Format_Grayscale8);

    double pixels[N][N];
    for (int y = 0; y < N; ++y) {
        const uchar *row = small.constScanLine(y);
        for (int x = 0; x < N; ++x)
            pixels[y][x] = static_cast<double>(row[x]);
    }

    // 2-D DCT: compute only the top-left K×K block
    double dct[K][K];
    const double pi_2N = M_PI / (2.0 * N);
    for (int u = 0; u < K; ++u) {
        double cu = (u == 0) ? M_SQRT1_2 : 1.0;
        for (int v = 0; v < K; ++v) {
            double cv = (v == 0) ? M_SQRT1_2 : 1.0;
            double sum = 0.0;
            for (int y = 0; y < N; ++y) {
                double cy_cos = std::cos((2 * y + 1) * v * pi_2N);
                for (int x = 0; x < N; ++x)
                    sum += pixels[y][x]
                           * std::cos((2 * x + 1) * u * pi_2N)
                           * cy_cos;
            }
            dct[u][v] = (2.0 / N) * cu * cv * sum;
        }
    }

    // Flatten to array and find the median
    double vals[K * K];
    for (int u = 0; u < K; ++u)
        for (int v = 0; v < K; ++v)
            vals[u * K + v] = dct[u][v];

    double sorted[K * K];
    std::copy(vals, vals + K * K, sorted);
    std::sort(sorted, sorted + K * K);
    double median = (sorted[K * K / 2 - 1] + sorted[K * K / 2]) / 2.0;

    // Build hash: bit i set if vals[i] > median
    quint64 hash = 0;
    for (int i = 0; i < K * K; ++i) {
        if (vals[i] > median)
            hash |= (quint64(1) << i);
    }

    return hash;
}

/*! \brief Returns the Hamming distance between two pHash values.
 *
 * Counts the number of bit positions that differ.
 *
 * \param a First hash.
 * \param b Second hash.
 * \return Number of differing bits (0–64).
 */
int PerceptualHasher::hammingDistance(quint64 a, quint64 b)
{
    return __builtin_popcountll(a ^ b);
}
