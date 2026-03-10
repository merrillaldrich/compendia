#ifndef PERCEPTUALHASHER_H
#define PERCEPTUALHASHER_H

#include <QImage>
#include <QtGlobal>

/*! \brief Provides perceptual (DCT-based) image hashing.
 *
 * Implements a 64-bit perceptual hash (pHash) using a discrete cosine
 * transform of a 32×32 greyscale thumbnail.  Two images with a Hamming
 * distance ≤ 10 bits are likely near-duplicates.
 */
class PerceptualHasher
{
public:
    /*! \brief Computes a 64-bit perceptual hash for the given image.
     *
     * Scales the image to 32×32 greyscale, applies a 2-D DCT, selects the
     * top-left 8×8 low-frequency block, and compares each coefficient against
     * the block median to produce a 64-bit hash.
     *
     * \param image Source image (any size or format).
     * \return 64-bit pHash value, or 0 if the image is null.
     */
    static quint64 computeHash(const QImage &image);

    /*! \brief Returns the Hamming distance between two pHash values.
     *
     * Counts the number of bit positions that differ.
     *
     * \param a First hash.
     * \param b Second hash.
     * \return Number of differing bits (0–64).
     */
    static int hammingDistance(quint64 a, quint64 b);
};

#endif // PERCEPTUALHASHER_H
