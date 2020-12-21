#include "feature_SIFT.hpp"

#include "xcv/xprecomp.hpp"
#include "xcv/xopencl_kernels_imgproc.hpp"

#include "xcv/xovx_defs.hpp"

#include <opencv2/core/hal/hal.hpp>

#include <cstdio>
#include <vector>
#include <iostream>
#include <functional>

using namespace cv;

// #ifdef CV_CXX11
// struct greaterThanPtr
// #else
// struct greaterThanPtr : public std::binary_function<const float *, const float *, bool>
// #endif
// {
//     bool operator()(const float *a, const float *b) const
//     // Ensure a fully deterministic result of the sort
//     {
//         return (*a > *b) ? true : (*a < *b) ? false : (a > b);
//     }
// };

// void getGoodFeatures(InputArray _image, OutputArray _corners,
//                      int maxCorners, double qualityLevel, double minDistance,
//                      InputArray _mask, int blockSize, int gradientSize,
//                      bool useHarrisDetector, double harrisK)
// {
//     CV_INSTRUMENT_REGION()

//     CV_Assert(qualityLevel > 0 && minDistance >= 0 && maxCorners >= 0);
//     CV_Assert(_mask.empty() || (_mask.type() == CV_8UC1 && _mask.sameSize(_image)));

//     // CV_OCL_RUN(_image.dims() <= 2 && _image.isUMat(),
//     //            ocl_goodFeaturesToTrack(_image, _corners, maxCorners, qualityLevel, minDistance,
//     //                                    _mask, blockSize, gradientSize, useHarrisDetector, harrisK))

//     Mat image = _image.getMat(), eig, tmp;
//     if (image.empty())
//     {
//         _corners.release();
//         return;
//     }

//     // Disabled due to bad accuracy
//     // CV_OVX_RUN(false && useHarrisDetector && _mask.empty() &&
//     //                !ovx::skipSmallImages<VX_KERNEL_HARRIS_CORNERS>(image.cols, image.rows),
//     //            openvx_harris(image, _corners, maxCorners, qualityLevel, minDistance, blockSize, gradientSize, harrisK))

//     if (useHarrisDetector)
//         cornerHarris(image, eig, blockSize, gradientSize, harrisK);
//     else
//         cornerMinEigenVal(image, eig, blockSize, gradientSize);

//     double maxVal = 0;
//     minMaxLoc(eig, 0, &maxVal, 0, 0, _mask);
//     threshold(eig, eig, maxVal * qualityLevel, 0, THRESH_TOZERO);
//     dilate(eig, tmp, Mat());

//     Size imgsize = image.size();
//     std::vector<const float *> tmpCorners;

//     // collect list of pointers to features - put them into temporary image
//     Mat mask = _mask.getMat();
//     for (int y = 1; y < imgsize.height - 1; y++)
//     {
//         const float *eig_data = (const float *)eig.ptr(y);
//         const float *tmp_data = (const float *)tmp.ptr(y);
//         const uchar *mask_data = mask.data ? mask.ptr(y) : 0;

//         for (int x = 1; x < imgsize.width - 1; x++)
//         {
//             float val = eig_data[x];
//             if (val != 0 && val == tmp_data[x] && (!mask_data || mask_data[x]))
//                 tmpCorners.push_back(eig_data + x);
//         }
//     }

//     std::vector<Point2f> corners;
//     size_t i, j, total = tmpCorners.size(), ncorners = 0;

//     if (total == 0)
//     {
//         _corners.release();
//         return;
//     }

//     std::sort(tmpCorners.begin(), tmpCorners.end(), greaterThanPtr());

//     if (minDistance >= 1)
//     {
//         // Partition the image into larger grids
//         int w = image.cols;
//         int h = image.rows;

//         const int cell_size = cvRound(minDistance);
//         const int grid_width = (w + cell_size - 1) / cell_size;
//         const int grid_height = (h + cell_size - 1) / cell_size;

//         std::vector<std::vector<Point2f>> grid(grid_width * grid_height);

//         minDistance *= minDistance;

//         for (i = 0; i < total; i++)
//         {
//             int ofs = (int)((const uchar *)tmpCorners[i] - eig.ptr());
//             int y = (int)(ofs / eig.step);
//             int x = (int)((ofs - y * eig.step) / sizeof(float));

//             bool good = true;

//             int x_cell = x / cell_size;
//             int y_cell = y / cell_size;

//             int x1 = x_cell - 1;
//             int y1 = y_cell - 1;
//             int x2 = x_cell + 1;
//             int y2 = y_cell + 1;

//             // boundary check
//             x1 = std::max(0, x1);
//             y1 = std::max(0, y1);
//             x2 = std::min(grid_width - 1, x2);
//             y2 = std::min(grid_height - 1, y2);

//             for (int yy = y1; yy <= y2; yy++)
//             {
//                 for (int xx = x1; xx <= x2; xx++)
//                 {
//                     std::vector<Point2f> &m = grid[yy * grid_width + xx];

//                     if (m.size())
//                     {
//                         for (j = 0; j < m.size(); j++)
//                         {
//                             float dx = x - m[j].x;
//                             float dy = y - m[j].y;

//                             if (dx * dx + dy * dy < minDistance)
//                             {
//                                 good = false;
//                                 goto break_out;
//                             }
//                         }
//                     }
//                 }
//             }

//         break_out:

//             if (good)
//             {
//                 grid[y_cell * grid_width + x_cell].push_back(Point2f((float)x, (float)y));

//                 corners.push_back(Point2f((float)x, (float)y));
//                 ++ncorners;

//                 if (maxCorners > 0 && (int)ncorners == maxCorners)
//                     break;
//             }
//         }
//     }
//     else
//     {
//         for (i = 0; i < total; i++)
//         {
//             int ofs = (int)((const uchar *)tmpCorners[i] - eig.ptr());
//             int y = (int)(ofs / eig.step);
//             int x = (int)((ofs - y * eig.step) / sizeof(float));

//             corners.push_back(Point2f((float)x, (float)y));
//             ++ncorners;
//             if (maxCorners > 0 && (int)ncorners == maxCorners)
//                 break;
//         }
//     }

//     Mat(corners).convertTo(_corners, _corners.fixedType() ? _corners.type() : CV_32F);
// }

// void getGoodFeatures(InputArray _image, OutputArray _corners,
//                      int maxCorners, double qualityLevel, double minDistance,
//                      InputArray _mask, int blockSize,
//                      bool useHarrisDetector, double harrisK)
// {
//     getGoodFeatures(_image, _corners, maxCorners, qualityLevel, minDistance,
//                     _mask, blockSize, 3, useHarrisDetector, harrisK);
// }

static inline void
unpackOctave(const KeyPoint &kpt, int &octave, int &layer, float &scale)
{
    octave = kpt.octave & 255;
    layer = (kpt.octave >> 8) & 255;
    octave = octave < 128 ? octave : (-128 | octave);
    scale = octave >= 0 ? 1.f / (1 << octave) : (float)(1 << -octave);
}

static Mat createInitialImage(const Mat &img, bool doubleImageSize, float sigma)
{
    Mat gray, gray_fpt;
    if (img.channels() == 3 || img.channels() == 4)
    {
        cvtColor(img, gray, COLOR_BGR2GRAY);
        gray.convertTo(gray_fpt, DataType<sift_wt>::type, SIFT_FIXPT_SCALE, 0);
    }
    else
        img.convertTo(gray_fpt, DataType<sift_wt>::type, SIFT_FIXPT_SCALE, 0);

    float sig_diff;

    if (doubleImageSize)
    {
        sig_diff = sqrtf(std::max(sigma * sigma - SIFT_INIT_SIGMA * SIFT_INIT_SIGMA * 4, 0.01f));
        Mat dbl;
#if DoG_TYPE_SHORT
        resize(gray_fpt, dbl, Size(gray_fpt.cols * 2, gray_fpt.rows * 2), 0, 0, INTER_LINEAR_EXACT);
#else
        resize(gray_fpt, dbl, Size(gray_fpt.cols * 2, gray_fpt.rows * 2), 0, 0, INTER_LINEAR);
#endif
        GaussianBlur(dbl, dbl, Size(), sig_diff, sig_diff);
        return dbl;
    }
    else
    {
        sig_diff = sqrtf(std::max(sigma * sigma - SIFT_INIT_SIGMA * SIFT_INIT_SIGMA, 0.01f));
        GaussianBlur(gray_fpt, gray_fpt, Size(), sig_diff, sig_diff);
        return gray_fpt;
    }
}

void buildGaussianPyramid(const Mat &base, std::vector<Mat> &pyr, int nOctaves)
{
    int nOctaveLayers = 3;
    double sigma = 1.6;

    std::vector<double>
        sig(nOctaveLayers + 3);
    pyr.resize(nOctaves * (nOctaveLayers + 3));

    // precompute Gaussian sigmas using the following formula:
    //  \sigma_{total}^2 = \sigma_{i}^2 + \sigma_{i-1}^2
    sig[0] = sigma;
    double k = std::pow(2., 1. / nOctaveLayers);
    for (int i = 1; i < nOctaveLayers + 3; i++)
    {
        double sig_prev = std::pow(k, (double)(i - 1)) * sigma;
        double sig_total = sig_prev * k;
        sig[i] = std::sqrt(sig_total * sig_total - sig_prev * sig_prev);
    }

    for (int o = 0; o < nOctaves; o++)
    {
        for (int i = 0; i < nOctaveLayers + 3; i++)
        {
            Mat &dst = pyr[o * (nOctaveLayers + 3) + i];
            if (o == 0 && i == 0)
                dst = base;
            // base of new octave is halved image from end of previous octave
            else if (i == 0)
            {
                const Mat &src = pyr[(o - 1) * (nOctaveLayers + 3) + nOctaveLayers];
                resize(src, dst, Size(src.cols / 2, src.rows / 2),
                       0, 0, INTER_NEAREST);
            }
            else
            {
                const Mat &src = pyr[o * (nOctaveLayers + 3) + i - 1];
                GaussianBlur(src, dst, Size(), sig[i], sig[i]);
            }
        }
    }
}

class buildDoGPyramidComputer : public ParallelLoopBody
{
public:
    buildDoGPyramidComputer(
        int _nOctaveLayers,
        const std::vector<Mat> &_gpyr,
        std::vector<Mat> &_dogpyr)
        : nOctaveLayers(_nOctaveLayers),
          gpyr(_gpyr),
          dogpyr(_dogpyr) {}

    void operator()(const cv::Range &range) const CV_OVERRIDE
    {
        const int begin = range.start;
        const int end = range.end;

        for (int a = begin; a < end; a++)
        {
            const int o = a / (nOctaveLayers + 2);
            const int i = a % (nOctaveLayers + 2);

            const Mat &src1 = gpyr[o * (nOctaveLayers + 3) + i];
            const Mat &src2 = gpyr[o * (nOctaveLayers + 3) + i + 1];
            Mat &dst = dogpyr[o * (nOctaveLayers + 2) + i];
            subtract(src2, src1, dst, noArray(), DataType<sift_wt>::type);
        }
    }

private:
    int nOctaveLayers;
    const std::vector<Mat> &gpyr;
    std::vector<Mat> &dogpyr;
};

void buildDoGPyramid(const std::vector<Mat> &gpyr, std::vector<Mat> &dogpyr)
{
    int nOctaveLayers = 3;
    int nOctaves = (int)gpyr.size() / (nOctaveLayers + 3);
    dogpyr.resize(nOctaves * (nOctaveLayers + 2));

    parallel_for_(Range(0, nOctaves * (nOctaveLayers + 2)), buildDoGPyramidComputer(nOctaveLayers, gpyr, dogpyr));
}

// Computes a gradient orientation histogram at a specified pixel
static float calcOrientationHist(const Mat &img, Point pt, int radius,
                                 float sigma, float *hist, int n)
{
    int i, j, k, len = (radius * 2 + 1) * (radius * 2 + 1);

    float expf_scale = -1.f / (2.f * sigma * sigma);
    AutoBuffer<float> buf(len * 4 + n + 4);
    float *X = buf, *Y = X + len, *Mag = X, *Ori = Y + len, *W = Ori + len;
    float *temphist = W + len + 2;

    for (i = 0; i < n; i++)
        temphist[i] = 0.f;

    for (i = -radius, k = 0; i <= radius; i++)
    {
        int y = pt.y + i;
        if (y <= 0 || y >= img.rows - 1)
            continue;
        for (j = -radius; j <= radius; j++)
        {
            int x = pt.x + j;
            if (x <= 0 || x >= img.cols - 1)
                continue;

            float dx = (float)(img.at<sift_wt>(y, x + 1) - img.at<sift_wt>(y, x - 1));
            float dy = (float)(img.at<sift_wt>(y - 1, x) - img.at<sift_wt>(y + 1, x));

            X[k] = dx;
            Y[k] = dy;
            W[k] = (i * i + j * j) * expf_scale;
            k++;
        }
    }

    len = k;

    // compute gradient values, orientations and the weights over the pixel neighborhood
    cv::hal::exp32f(W, W, len);
    cv::hal::fastAtan2(Y, X, Ori, len, true);
    cv::hal::magnitude32f(X, Y, Mag, len);

    k = 0;
#if CV_AVX2
    if (USE_AVX2)
    {
        __m256 __nd360 = _mm256_set1_ps(n / 360.f);
        __m256i __n = _mm256_set1_epi32(n);
        int CV_DECL_ALIGNED(32) bin_buf[8];
        float CV_DECL_ALIGNED(32) w_mul_mag_buf[8];
        for (; k <= len - 8; k += 8)
        {
            __m256i __bin = _mm256_cvtps_epi32(_mm256_mul_ps(__nd360, _mm256_loadu_ps(&Ori[k])));

            __bin = _mm256_sub_epi32(__bin, _mm256_andnot_si256(_mm256_cmpgt_epi32(__n, __bin), __n));
            __bin = _mm256_add_epi32(__bin, _mm256_and_si256(__n, _mm256_cmpgt_epi32(_mm256_setzero_si256(), __bin)));

            __m256 __w_mul_mag = _mm256_mul_ps(_mm256_loadu_ps(&W[k]), _mm256_loadu_ps(&Mag[k]));

            _mm256_store_si256((__m256i *)bin_buf, __bin);
            _mm256_store_ps(w_mul_mag_buf, __w_mul_mag);

            temphist[bin_buf[0]] += w_mul_mag_buf[0];
            temphist[bin_buf[1]] += w_mul_mag_buf[1];
            temphist[bin_buf[2]] += w_mul_mag_buf[2];
            temphist[bin_buf[3]] += w_mul_mag_buf[3];
            temphist[bin_buf[4]] += w_mul_mag_buf[4];
            temphist[bin_buf[5]] += w_mul_mag_buf[5];
            temphist[bin_buf[6]] += w_mul_mag_buf[6];
            temphist[bin_buf[7]] += w_mul_mag_buf[7];
        }
    }
#endif
    for (; k < len; k++)
    {
        int bin = cvRound((n / 360.f) * Ori[k]);
        if (bin >= n)
            bin -= n;
        if (bin < 0)
            bin += n;
        temphist[bin] += W[k] * Mag[k];
    }

    // smooth the histogram
    temphist[-1] = temphist[n - 1];
    temphist[-2] = temphist[n - 2];
    temphist[n] = temphist[0];
    temphist[n + 1] = temphist[1];

    i = 0;
#if CV_AVX2
    if (USE_AVX2)
    {
        __m256 __d_1_16 = _mm256_set1_ps(1.f / 16.f);
        __m256 __d_4_16 = _mm256_set1_ps(4.f / 16.f);
        __m256 __d_6_16 = _mm256_set1_ps(6.f / 16.f);
        for (; i <= n - 8; i += 8)
        {
#if CV_FMA3
            __m256 __hist = _mm256_fmadd_ps(
                _mm256_add_ps(_mm256_loadu_ps(&temphist[i - 2]), _mm256_loadu_ps(&temphist[i + 2])),
                __d_1_16,
                _mm256_fmadd_ps(
                    _mm256_add_ps(_mm256_loadu_ps(&temphist[i - 1]), _mm256_loadu_ps(&temphist[i + 1])),
                    __d_4_16,
                    _mm256_mul_ps(_mm256_loadu_ps(&temphist[i]), __d_6_16)));
#else
            __m256 __hist = _mm256_add_ps(
                _mm256_mul_ps(
                    _mm256_add_ps(_mm256_loadu_ps(&temphist[i - 2]), _mm256_loadu_ps(&temphist[i + 2])),
                    __d_1_16),
                _mm256_add_ps(
                    _mm256_mul_ps(
                        _mm256_add_ps(_mm256_loadu_ps(&temphist[i - 1]), _mm256_loadu_ps(&temphist[i + 1])),
                        __d_4_16),
                    _mm256_mul_ps(_mm256_loadu_ps(&temphist[i]), __d_6_16)));
#endif
            _mm256_storeu_ps(&hist[i], __hist);
        }
    }
#endif
    for (; i < n; i++)
    {
        hist[i] = (temphist[i - 2] + temphist[i + 2]) * (1.f / 16.f) +
                  (temphist[i - 1] + temphist[i + 1]) * (4.f / 16.f) +
                  temphist[i] * (6.f / 16.f);
    }

    float maxval = hist[0];
    for (i = 1; i < n; i++)
        maxval = std::max(maxval, hist[i]);

    return maxval;
}

//
// Interpolates a scale-space extremum's location and scale to subpixel
// accuracy to form an image feature. Rejects features with low contrast.
// Based on Section 4 of Lowe's paper.
static bool adjustLocalExtrema(const std::vector<Mat> &dog_pyr, KeyPoint &kpt, int octv,
                               int &layer, int &r, int &c, int nOctaveLayers,
                               float contrastThreshold, float edgeThreshold, float sigma)
{
    const float img_scale = 1.f / (255 * SIFT_FIXPT_SCALE);
    const float deriv_scale = img_scale * 0.5f;
    const float second_deriv_scale = img_scale;
    const float cross_deriv_scale = img_scale * 0.25f;

    float xi = 0, xr = 0, xc = 0, contr = 0;
    int i = 0;

    for (; i < SIFT_MAX_INTERP_STEPS; i++)
    {
        int idx = octv * (nOctaveLayers + 2) + layer;
        const Mat &img = dog_pyr[idx];
        const Mat &prev = dog_pyr[idx - 1];
        const Mat &next = dog_pyr[idx + 1];

        Vec3f dD((img.at<sift_wt>(r, c + 1) - img.at<sift_wt>(r, c - 1)) * deriv_scale,
                 (img.at<sift_wt>(r + 1, c) - img.at<sift_wt>(r - 1, c)) * deriv_scale,
                 (next.at<sift_wt>(r, c) - prev.at<sift_wt>(r, c)) * deriv_scale);

        float v2 = (float)img.at<sift_wt>(r, c) * 2;
        float dxx = (img.at<sift_wt>(r, c + 1) + img.at<sift_wt>(r, c - 1) - v2) * second_deriv_scale;
        float dyy = (img.at<sift_wt>(r + 1, c) + img.at<sift_wt>(r - 1, c) - v2) * second_deriv_scale;
        float dss = (next.at<sift_wt>(r, c) + prev.at<sift_wt>(r, c) - v2) * second_deriv_scale;
        float dxy = (img.at<sift_wt>(r + 1, c + 1) - img.at<sift_wt>(r + 1, c - 1) -
                     img.at<sift_wt>(r - 1, c + 1) + img.at<sift_wt>(r - 1, c - 1)) *
                    cross_deriv_scale;
        float dxs = (next.at<sift_wt>(r, c + 1) - next.at<sift_wt>(r, c - 1) -
                     prev.at<sift_wt>(r, c + 1) + prev.at<sift_wt>(r, c - 1)) *
                    cross_deriv_scale;
        float dys = (next.at<sift_wt>(r + 1, c) - next.at<sift_wt>(r - 1, c) -
                     prev.at<sift_wt>(r + 1, c) + prev.at<sift_wt>(r - 1, c)) *
                    cross_deriv_scale;

        Matx33f H(dxx, dxy, dxs,
                  dxy, dyy, dys,
                  dxs, dys, dss);

        Vec3f X = H.solve(dD, DECOMP_LU);

        xi = -X[2];
        xr = -X[1];
        xc = -X[0];

        if (std::abs(xi) < 0.5f && std::abs(xr) < 0.5f && std::abs(xc) < 0.5f)
            break;

        if (std::abs(xi) > (float)(INT_MAX / 3) ||
            std::abs(xr) > (float)(INT_MAX / 3) ||
            std::abs(xc) > (float)(INT_MAX / 3))
            return false;

        c += cvRound(xc);
        r += cvRound(xr);
        layer += cvRound(xi);

        if (layer < 1 || layer > nOctaveLayers ||
            c < SIFT_IMG_BORDER || c >= img.cols - SIFT_IMG_BORDER ||
            r < SIFT_IMG_BORDER || r >= img.rows - SIFT_IMG_BORDER)
            return false;
    }

    // ensure convergence of interpolation
    if (i >= SIFT_MAX_INTERP_STEPS)
        return false;

    {
        int idx = octv * (nOctaveLayers + 2) + layer;
        const Mat &img = dog_pyr[idx];
        const Mat &prev = dog_pyr[idx - 1];
        const Mat &next = dog_pyr[idx + 1];
        Matx31f dD((img.at<sift_wt>(r, c + 1) - img.at<sift_wt>(r, c - 1)) * deriv_scale,
                   (img.at<sift_wt>(r + 1, c) - img.at<sift_wt>(r - 1, c)) * deriv_scale,
                   (next.at<sift_wt>(r, c) - prev.at<sift_wt>(r, c)) * deriv_scale);
        float t = dD.dot(Matx31f(xc, xr, xi));

        contr = img.at<sift_wt>(r, c) * img_scale + t * 0.5f;
        if (std::abs(contr) * nOctaveLayers < contrastThreshold)
            return false;

        // principal curvatures are computed using the trace and det of Hessian
        float v2 = img.at<sift_wt>(r, c) * 2.f;
        float dxx = (img.at<sift_wt>(r, c + 1) + img.at<sift_wt>(r, c - 1) - v2) * second_deriv_scale;
        float dyy = (img.at<sift_wt>(r + 1, c) + img.at<sift_wt>(r - 1, c) - v2) * second_deriv_scale;
        float dxy = (img.at<sift_wt>(r + 1, c + 1) - img.at<sift_wt>(r + 1, c - 1) -
                     img.at<sift_wt>(r - 1, c + 1) + img.at<sift_wt>(r - 1, c - 1)) *
                    cross_deriv_scale;
        float tr = dxx + dyy;
        float det = dxx * dyy - dxy * dxy;

        if (det <= 0 || tr * tr * edgeThreshold >= (edgeThreshold + 1) * (edgeThreshold + 1) * det)
            return false;
    }

    kpt.pt.x = (c + xc) * (1 << octv);
    kpt.pt.y = (r + xr) * (1 << octv);
    kpt.octave = octv + (layer << 8) + (cvRound((xi + 0.5) * 255) << 16);
    kpt.size = sigma * powf(2.f, (layer + xi) / nOctaveLayers) * (1 << octv) * 2;
    kpt.response = std::abs(contr);

    return true;
}

class findScaleSpaceExtremaComputer : public ParallelLoopBody
{
public:
    findScaleSpaceExtremaComputer(
        int _o,
        int _i,
        int _threshold,
        int _idx,
        int _step,
        int _cols,
        int _nOctaveLayers,
        double _contrastThreshold,
        double _edgeThreshold,
        double _sigma,
        const std::vector<Mat> &_gauss_pyr,
        const std::vector<Mat> &_dog_pyr,
        TLSData<std::vector<KeyPoint>> &_tls_kpts_struct)

        : o(_o),
          i(_i),
          threshold(_threshold),
          idx(_idx),
          step(_step),
          cols(_cols),
          nOctaveLayers(_nOctaveLayers),
          contrastThreshold(_contrastThreshold),
          edgeThreshold(_edgeThreshold),
          sigma(_sigma),
          gauss_pyr(_gauss_pyr),
          dog_pyr(_dog_pyr),
          tls_kpts_struct(_tls_kpts_struct)
    {
    }
    void operator()(const cv::Range &range) const CV_OVERRIDE
    {
        const int begin = range.start;
        const int end = range.end;

        static const int n = SIFT_ORI_HIST_BINS;
        float hist[n];

        const Mat &img = dog_pyr[idx];
        const Mat &prev = dog_pyr[idx - 1];
        const Mat &next = dog_pyr[idx + 1];

        std::vector<KeyPoint> *tls_kpts = tls_kpts_struct.get();

        KeyPoint kpt;
        for (int r = begin; r < end; r++)
        {
            const sift_wt *currptr = img.ptr<sift_wt>(r);
            const sift_wt *prevptr = prev.ptr<sift_wt>(r);
            const sift_wt *nextptr = next.ptr<sift_wt>(r);

            for (int c = SIFT_IMG_BORDER; c < cols - SIFT_IMG_BORDER; c++)
            {
                sift_wt val = currptr[c];

                // find local extrema with pixel accuracy
                if (std::abs(val) > threshold &&
                    ((val > 0 && val >= currptr[c - 1] && val >= currptr[c + 1] &&
                      val >= currptr[c - step - 1] && val >= currptr[c - step] && val >= currptr[c - step + 1] &&
                      val >= currptr[c + step - 1] && val >= currptr[c + step] && val >= currptr[c + step + 1] &&
                      val >= nextptr[c] && val >= nextptr[c - 1] && val >= nextptr[c + 1] &&
                      val >= nextptr[c - step - 1] && val >= nextptr[c - step] && val >= nextptr[c - step + 1] &&
                      val >= nextptr[c + step - 1] && val >= nextptr[c + step] && val >= nextptr[c + step + 1] &&
                      val >= prevptr[c] && val >= prevptr[c - 1] && val >= prevptr[c + 1] &&
                      val >= prevptr[c - step - 1] && val >= prevptr[c - step] && val >= prevptr[c - step + 1] &&
                      val >= prevptr[c + step - 1] && val >= prevptr[c + step] && val >= prevptr[c + step + 1]) ||
                     (val < 0 && val <= currptr[c - 1] && val <= currptr[c + 1] &&
                      val <= currptr[c - step - 1] && val <= currptr[c - step] && val <= currptr[c - step + 1] &&
                      val <= currptr[c + step - 1] && val <= currptr[c + step] && val <= currptr[c + step + 1] &&
                      val <= nextptr[c] && val <= nextptr[c - 1] && val <= nextptr[c + 1] &&
                      val <= nextptr[c - step - 1] && val <= nextptr[c - step] && val <= nextptr[c - step + 1] &&
                      val <= nextptr[c + step - 1] && val <= nextptr[c + step] && val <= nextptr[c + step + 1] &&
                      val <= prevptr[c] && val <= prevptr[c - 1] && val <= prevptr[c + 1] &&
                      val <= prevptr[c - step - 1] && val <= prevptr[c - step] && val <= prevptr[c - step + 1] &&
                      val <= prevptr[c + step - 1] && val <= prevptr[c + step] && val <= prevptr[c + step + 1])))
                {
                    int r1 = r, c1 = c, layer = i;
                    if (!adjustLocalExtrema(dog_pyr, kpt, o, layer, r1, c1,
                                            nOctaveLayers, (float)contrastThreshold,
                                            (float)edgeThreshold, (float)sigma))
                        continue;
                    float scl_octv = kpt.size * 0.5f / (1 << o);
                    float omax = calcOrientationHist(gauss_pyr[o * (nOctaveLayers + 3) + layer],
                                                     Point(c1, r1),
                                                     cvRound(SIFT_ORI_RADIUS * scl_octv),
                                                     SIFT_ORI_SIG_FCTR * scl_octv,
                                                     hist, n);
                    float mag_thr = (float)(omax * SIFT_ORI_PEAK_RATIO);
                    for (int j = 0; j < n; j++)
                    {
                        int l = j > 0 ? j - 1 : n - 1;
                        int r2 = j < n - 1 ? j + 1 : 0;

                        if (hist[j] > hist[l] && hist[j] > hist[r2] && hist[j] >= mag_thr)
                        {
                            float bin = j + 0.5f * (hist[l] - hist[r2]) / (hist[l] - 2 * hist[j] + hist[r2]);
                            bin = bin < 0 ? n + bin : bin >= n ? bin - n : bin;
                            kpt.angle = 360.f - (float)((360.f / n) * bin);
                            if (std::abs(kpt.angle - 360.f) < FLT_EPSILON)
                                kpt.angle = 0.f;
                            {
                                tls_kpts->push_back(kpt);
                            }
                        }
                    }
                }
            }
        }
    }

private:
    int o, i;
    int threshold;
    int idx, step, cols;
    int nOctaveLayers;
    double contrastThreshold;
    double edgeThreshold;
    double sigma;
    const std::vector<Mat> &gauss_pyr;
    const std::vector<Mat> &dog_pyr;
    TLSData<std::vector<KeyPoint>> &tls_kpts_struct;
};

//
// Detects features at extrema in DoG scale space.  Bad features are discarded
// based on contrast and ratio of principal curvatures.
void findScaleSpaceExtrema(const std::vector<Mat> &gauss_pyr, const std::vector<Mat> &dog_pyr,
                           std::vector<KeyPoint> &keypoints)
{
    int nOctaveLayers = 3;
    double contrastThreshold = 0.04;
    double edgeThreshold = 10;
    double sigma = 1.6;

    const int nOctaves = (int)gauss_pyr.size() / (nOctaveLayers + 3);
    const int threshold = cvFloor(0.5 * contrastThreshold / nOctaveLayers * 255 * SIFT_FIXPT_SCALE);

    keypoints.clear();
    TLSData<std::vector<KeyPoint>> tls_kpts_struct;

    for (int o = 0; o < nOctaves; o++)
        for (int i = 1; i <= nOctaveLayers; i++)
        {
            const int idx = o * (nOctaveLayers + 2) + i;
            const Mat &img = dog_pyr[idx];
            const int step = (int)img.step1();
            const int rows = img.rows, cols = img.cols;

            parallel_for_(Range(SIFT_IMG_BORDER, rows - SIFT_IMG_BORDER),
                          findScaleSpaceExtremaComputer(
                              o, i, threshold, idx, step, cols,
                              nOctaveLayers,
                              contrastThreshold,
                              edgeThreshold,
                              sigma,
                              gauss_pyr, dog_pyr, tls_kpts_struct));
        }

    std::vector<std::vector<KeyPoint> *> kpt_vecs;
    tls_kpts_struct.gather(kpt_vecs);
    for (size_t i = 0; i < kpt_vecs.size(); ++i)
    {
        keypoints.insert(keypoints.end(), kpt_vecs[i]->begin(), kpt_vecs[i]->end());
    }
}

void calKeyPointbySIFT(InputArray _image,
                       std::vector<cv::Point2f> corners,
                       std::vector<KeyPoint> &keypoints)
{
    double sigma = 1.6;
    int nfeatures = 0;

    if (_image.empty())
    {
        keypoints.clear();
        return;
    }

    int firstOctave = -1, actualNOctaves = 0, actualNLayers = 0;
    Mat image = _image.getMat();

    if (image.empty() || image.depth() != CV_8U)
        CV_Error(Error::StsBadArg, "image is empty or has incorrect depth (!=CV_8U)");

    cv::Mat base = createInitialImage(image, firstOctave < 0, (float)sigma);

    std::vector<Mat> gpyr, dogpyr;
    int nOctaves = actualNOctaves > 0 ? actualNOctaves : cvRound(std::log((double)std::min(base.cols, base.rows)) / std::log(2.) - 2) - firstOctave;

    //double t, tf = getTickFrequency();
    //t = (double)getTickCount();
    buildGaussianPyramid(base, gpyr, nOctaves);
    buildDoGPyramid(gpyr, dogpyr);
    // cv::Mat a;
    // for (int i = 0; i < gpyr.size(); i++)
    // {
    //     a = gpyr[i];
    //     int j = 10;
    // }

    //t = (double)getTickCount();
    findScaleSpaceExtrema(gpyr, dogpyr, keypoints);
    KeyPointsFilter::removeDuplicatedSorted(keypoints);

    if (nfeatures > 0)
        KeyPointsFilter::retainBest(keypoints, nfeatures);
    //t = (double)getTickCount() - t;
    //printf("keypoint detection time: %g\n", t*1000./tf);

    if (firstOctave < 0)
        for (size_t i = 0; i < keypoints.size(); i++)
        {
            KeyPoint &kpt = keypoints[i];
            float scale = 1.f / (float)(1 << -firstOctave);
            kpt.octave = (kpt.octave & ~255) | ((kpt.octave + firstOctave) & 255);
            kpt.pt *= scale;
            kpt.size *= scale;
        }
}

static void calcSIFTDescriptor(const Mat &img, Point2f ptf, float ori, float scl,
                               int d, int n, float *dst)
{
    Point pt(cvRound(ptf.x), cvRound(ptf.y));
    float cos_t = cosf(ori * (float)(CV_PI / 180));
    float sin_t = sinf(ori * (float)(CV_PI / 180));
    float bins_per_rad = n / 360.f;
    float exp_scale = -1.f / (d * d * 0.5f);
    float hist_width = SIFT_DESCR_SCL_FCTR * scl;
    int radius = cvRound(hist_width * 1.4142135623730951f * (d + 1) * 0.5f);
    // Clip the radius to the diagonal of the image to avoid autobuffer too large exception
    radius = std::min(radius, (int)sqrt(((double)img.cols) * img.cols + ((double)img.rows) * img.rows));
    cos_t /= hist_width;
    sin_t /= hist_width;

    int i, j, k, len = (radius * 2 + 1) * (radius * 2 + 1), histlen = (d + 2) * (d + 2) * (n + 2);
    int rows = img.rows, cols = img.cols;

    AutoBuffer<float> buf(len * 6 + histlen);
    float *X = buf, *Y = X + len, *Mag = Y, *Ori = Mag + len, *W = Ori + len;
    float *RBin = W + len, *CBin = RBin + len, *hist = CBin + len;

    for (i = 0; i < d + 2; i++)
    {
        for (j = 0; j < d + 2; j++)
            for (k = 0; k < n + 2; k++)
                hist[(i * (d + 2) + j) * (n + 2) + k] = 0.;
    }

    for (i = -radius, k = 0; i <= radius; i++)
        for (j = -radius; j <= radius; j++)
        {
            // Calculate sample's histogram array coords rotated relative to ori.
            // Subtract 0.5 so samples that fall e.g. in the center of row 1 (i.e.
            // r_rot = 1.5) have full weight placed in row 1 after interpolation.
            float c_rot = j * cos_t - i * sin_t;
            float r_rot = j * sin_t + i * cos_t;
            float rbin = r_rot + d / 2 - 0.5f;
            float cbin = c_rot + d / 2 - 0.5f;
            int r = pt.y + i, c = pt.x + j;

            if (rbin > -1 && rbin < d && cbin > -1 && cbin < d &&
                r > 0 && r < rows - 1 && c > 0 && c < cols - 1)
            {
                float dx = (float)(img.at<sift_wt>(r, c + 1) - img.at<sift_wt>(r, c - 1));
                float dy = (float)(img.at<sift_wt>(r - 1, c) - img.at<sift_wt>(r + 1, c));
                X[k] = dx;
                Y[k] = dy;
                RBin[k] = rbin;
                CBin[k] = cbin;
                W[k] = (c_rot * c_rot + r_rot * r_rot) * exp_scale;
                k++;
            }
        }

    len = k;
    cv::hal::fastAtan2(Y, X, Ori, len, true);
    cv::hal::magnitude32f(X, Y, Mag, len);
    cv::hal::exp32f(W, W, len);

    k = 0;
#if CV_AVX2
    if (USE_AVX2)
    {
        int CV_DECL_ALIGNED(32) idx_buf[8];
        float CV_DECL_ALIGNED(32) rco_buf[64];
        const __m256 __ori = _mm256_set1_ps(ori);
        const __m256 __bins_per_rad = _mm256_set1_ps(bins_per_rad);
        const __m256i __n = _mm256_set1_epi32(n);
        for (; k <= len - 8; k += 8)
        {
            __m256 __rbin = _mm256_loadu_ps(&RBin[k]);
            __m256 __cbin = _mm256_loadu_ps(&CBin[k]);
            __m256 __obin = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(&Ori[k]), __ori), __bins_per_rad);
            __m256 __mag = _mm256_mul_ps(_mm256_loadu_ps(&Mag[k]), _mm256_loadu_ps(&W[k]));

            __m256 __r0 = _mm256_floor_ps(__rbin);
            __rbin = _mm256_sub_ps(__rbin, __r0);
            __m256 __c0 = _mm256_floor_ps(__cbin);
            __cbin = _mm256_sub_ps(__cbin, __c0);
            __m256 __o0 = _mm256_floor_ps(__obin);
            __obin = _mm256_sub_ps(__obin, __o0);

            __m256i __o0i = _mm256_cvtps_epi32(__o0);
            __o0i = _mm256_add_epi32(__o0i, _mm256_and_si256(__n, _mm256_cmpgt_epi32(_mm256_setzero_si256(), __o0i)));
            __o0i = _mm256_sub_epi32(__o0i, _mm256_andnot_si256(_mm256_cmpgt_epi32(__n, __o0i), __n));

            __m256 __v_r1 = _mm256_mul_ps(__mag, __rbin);
            __m256 __v_r0 = _mm256_sub_ps(__mag, __v_r1);

            __m256 __v_rc11 = _mm256_mul_ps(__v_r1, __cbin);
            __m256 __v_rc10 = _mm256_sub_ps(__v_r1, __v_rc11);

            __m256 __v_rc01 = _mm256_mul_ps(__v_r0, __cbin);
            __m256 __v_rc00 = _mm256_sub_ps(__v_r0, __v_rc01);

            __m256 __v_rco111 = _mm256_mul_ps(__v_rc11, __obin);
            __m256 __v_rco110 = _mm256_sub_ps(__v_rc11, __v_rco111);

            __m256 __v_rco101 = _mm256_mul_ps(__v_rc10, __obin);
            __m256 __v_rco100 = _mm256_sub_ps(__v_rc10, __v_rco101);

            __m256 __v_rco011 = _mm256_mul_ps(__v_rc01, __obin);
            __m256 __v_rco010 = _mm256_sub_ps(__v_rc01, __v_rco011);

            __m256 __v_rco001 = _mm256_mul_ps(__v_rc00, __obin);
            __m256 __v_rco000 = _mm256_sub_ps(__v_rc00, __v_rco001);

            __m256i __one = _mm256_set1_epi32(1);
            __m256i __idx = _mm256_add_epi32(
                _mm256_mullo_epi32(
                    _mm256_add_epi32(
                        _mm256_mullo_epi32(_mm256_add_epi32(_mm256_cvtps_epi32(__r0), __one), _mm256_set1_epi32(d + 2)),
                        _mm256_add_epi32(_mm256_cvtps_epi32(__c0), __one)),
                    _mm256_set1_epi32(n + 2)),
                __o0i);

            _mm256_store_si256((__m256i *)idx_buf, __idx);

            _mm256_store_ps(&(rco_buf[0]), __v_rco000);
            _mm256_store_ps(&(rco_buf[8]), __v_rco001);
            _mm256_store_ps(&(rco_buf[16]), __v_rco010);
            _mm256_store_ps(&(rco_buf[24]), __v_rco011);
            _mm256_store_ps(&(rco_buf[32]), __v_rco100);
            _mm256_store_ps(&(rco_buf[40]), __v_rco101);
            _mm256_store_ps(&(rco_buf[48]), __v_rco110);
            _mm256_store_ps(&(rco_buf[56]), __v_rco111);
#define HIST_SUM_HELPER(id)                                            \
    hist[idx_buf[(id)]] += rco_buf[(id)];                              \
    hist[idx_buf[(id)] + 1] += rco_buf[8 + (id)];                      \
    hist[idx_buf[(id)] + (n + 2)] += rco_buf[16 + (id)];               \
    hist[idx_buf[(id)] + (n + 3)] += rco_buf[24 + (id)];               \
    hist[idx_buf[(id)] + (d + 2) * (n + 2)] += rco_buf[32 + (id)];     \
    hist[idx_buf[(id)] + (d + 2) * (n + 2) + 1] += rco_buf[40 + (id)]; \
    hist[idx_buf[(id)] + (d + 3) * (n + 2)] += rco_buf[48 + (id)];     \
    hist[idx_buf[(id)] + (d + 3) * (n + 2) + 1] += rco_buf[56 + (id)];

            HIST_SUM_HELPER(0);
            HIST_SUM_HELPER(1);
            HIST_SUM_HELPER(2);
            HIST_SUM_HELPER(3);
            HIST_SUM_HELPER(4);
            HIST_SUM_HELPER(5);
            HIST_SUM_HELPER(6);
            HIST_SUM_HELPER(7);

#undef HIST_SUM_HELPER
        }
    }
#endif
    for (; k < len; k++)
    {
        float rbin = RBin[k], cbin = CBin[k];
        float obin = (Ori[k] - ori) * bins_per_rad;
        float mag = Mag[k] * W[k];

        int r0 = cvFloor(rbin);
        int c0 = cvFloor(cbin);
        int o0 = cvFloor(obin);
        rbin -= r0;
        cbin -= c0;
        obin -= o0;

        if (o0 < 0)
            o0 += n;
        if (o0 >= n)
            o0 -= n;

        // histogram update using tri-linear interpolation
        float v_r1 = mag * rbin, v_r0 = mag - v_r1;
        float v_rc11 = v_r1 * cbin, v_rc10 = v_r1 - v_rc11;
        float v_rc01 = v_r0 * cbin, v_rc00 = v_r0 - v_rc01;
        float v_rco111 = v_rc11 * obin, v_rco110 = v_rc11 - v_rco111;
        float v_rco101 = v_rc10 * obin, v_rco100 = v_rc10 - v_rco101;
        float v_rco011 = v_rc01 * obin, v_rco010 = v_rc01 - v_rco011;
        float v_rco001 = v_rc00 * obin, v_rco000 = v_rc00 - v_rco001;

        int idx = ((r0 + 1) * (d + 2) + c0 + 1) * (n + 2) + o0;
        hist[idx] += v_rco000;
        hist[idx + 1] += v_rco001;
        hist[idx + (n + 2)] += v_rco010;
        hist[idx + (n + 3)] += v_rco011;
        hist[idx + (d + 2) * (n + 2)] += v_rco100;
        hist[idx + (d + 2) * (n + 2) + 1] += v_rco101;
        hist[idx + (d + 3) * (n + 2)] += v_rco110;
        hist[idx + (d + 3) * (n + 2) + 1] += v_rco111;
    }

    // finalize histogram, since the orientation histograms are circular
    for (i = 0; i < d; i++)
        for (j = 0; j < d; j++)
        {
            int idx = ((i + 1) * (d + 2) + (j + 1)) * (n + 2);
            hist[idx] += hist[idx + n];
            hist[idx + 1] += hist[idx + n + 1];
            for (k = 0; k < n; k++)
                dst[(i * d + j) * n + k] = hist[idx + k];
        }
    // copy histogram to the descriptor,
    // apply hysteresis thresholding
    // and scale the result, so that it can be easily converted
    // to byte array
    float nrm2 = 0;
    len = d * d * n;
    k = 0;
#if CV_AVX2
    if (USE_AVX2)
    {
        float CV_DECL_ALIGNED(32) nrm2_buf[8];
        __m256 __nrm2 = _mm256_setzero_ps();
        __m256 __dst;
        for (; k <= len - 8; k += 8)
        {
            __dst = _mm256_loadu_ps(&dst[k]);
#if CV_FMA3
            __nrm2 = _mm256_fmadd_ps(__dst, __dst, __nrm2);
#else
            __nrm2 = _mm256_add_ps(__nrm2, _mm256_mul_ps(__dst, __dst));
#endif
        }
        _mm256_store_ps(nrm2_buf, __nrm2);
        nrm2 = nrm2_buf[0] + nrm2_buf[1] + nrm2_buf[2] + nrm2_buf[3] +
               nrm2_buf[4] + nrm2_buf[5] + nrm2_buf[6] + nrm2_buf[7];
    }
#endif
    for (; k < len; k++)
        nrm2 += dst[k] * dst[k];

    float thr = std::sqrt(nrm2) * SIFT_DESCR_MAG_THR;

    i = 0, nrm2 = 0;
#if 0 //CV_AVX2
    // This code cannot be enabled because it sums nrm2 in a different order,
    // thus producing slightly different results
    if( USE_AVX2 )
    {
        float CV_DECL_ALIGNED(32) nrm2_buf[8];
        __m256 __dst;
        __m256 __nrm2 = _mm256_setzero_ps();
        __m256 __thr = _mm256_set1_ps(thr);
        for( ; i <= len - 8; i += 8 )
        {
            __dst = _mm256_loadu_ps(&dst[i]);
            __dst = _mm256_min_ps(__dst, __thr);
            _mm256_storeu_ps(&dst[i], __dst);
#if CV_FMA3
            __nrm2 = _mm256_fmadd_ps(__dst, __dst, __nrm2);
#else
            __nrm2 = _mm256_add_ps(__nrm2, _mm256_mul_ps(__dst, __dst));
#endif
        }
        _mm256_store_ps(nrm2_buf, __nrm2);
        nrm2 = nrm2_buf[0] + nrm2_buf[1] + nrm2_buf[2] + nrm2_buf[3] +
               nrm2_buf[4] + nrm2_buf[5] + nrm2_buf[6] + nrm2_buf[7];
    }
#endif
    for (; i < len; i++)
    {
        float val = std::min(dst[i], thr);
        dst[i] = val;
        nrm2 += val * val;
    }
    nrm2 = SIFT_INT_DESCR_FCTR / std::max(std::sqrt(nrm2), FLT_EPSILON);

#if 1
    k = 0;
#if CV_AVX2
    if (USE_AVX2)
    {
        __m256 __dst;
        __m256 __min = _mm256_setzero_ps();
        __m256 __max = _mm256_set1_ps(255.0f); // max of uchar
        __m256 __nrm2 = _mm256_set1_ps(nrm2);
        for (k = 0; k <= len - 8; k += 8)
        {
            __dst = _mm256_loadu_ps(&dst[k]);
            __dst = _mm256_min_ps(_mm256_max_ps(_mm256_round_ps(_mm256_mul_ps(__dst, __nrm2), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), __min), __max);
            _mm256_storeu_ps(&dst[k], __dst);
        }
    }
#endif
    for (; k < len; k++)
    {
        dst[k] = saturate_cast<uchar>(dst[k] * nrm2);
    }
#else
    float nrm1 = 0;
    for (k = 0; k < len; k++)
    {
        dst[k] *= nrm2;
        nrm1 += dst[k];
    }
    nrm1 = 1.f / std::max(nrm1, FLT_EPSILON);
    for (k = 0; k < len; k++)
    {
        dst[k] = std::sqrt(dst[k] * nrm1); //saturate_cast<uchar>(std::sqrt(dst[k] * nrm1)*SIFT_INT_DESCR_FCTR);
    }
#endif
}

class calcDescriptorsComputer : public ParallelLoopBody
{
public:
    calcDescriptorsComputer(const std::vector<Mat> &_gpyr,
                            const std::vector<KeyPoint> &_keypoints,
                            Mat &_descriptors,
                            int _nOctaveLayers,
                            int _firstOctave)
        : gpyr(_gpyr),
          keypoints(_keypoints),
          descriptors(_descriptors),
          nOctaveLayers(_nOctaveLayers),
          firstOctave(_firstOctave) {}

    void operator()(const cv::Range &range) const CV_OVERRIDE
    {
        const int begin = range.start;
        const int end = range.end;

        static const int d = SIFT_DESCR_WIDTH, n = SIFT_DESCR_HIST_BINS;

        for (int i = begin; i < end; i++)
        {
            KeyPoint kpt = keypoints[i];
            int octave, layer;
            float scale;
            unpackOctave(kpt, octave, layer, scale);
            CV_Assert(octave >= firstOctave && layer <= nOctaveLayers + 2);
            float size = kpt.size * scale;
            Point2f ptf(kpt.pt.x * scale, kpt.pt.y * scale);
            const Mat &img = gpyr[(octave - firstOctave) * (nOctaveLayers + 3) + layer];

            float angle = 360.f - kpt.angle;
            if (std::abs(angle - 360.f) < FLT_EPSILON)
                angle = 0.f;
            calcSIFTDescriptor(img, ptf, angle, size * 0.5f, d, n, descriptors.ptr<float>((int)i));
        }
    }

private:
    const std::vector<Mat> &gpyr;
    const std::vector<KeyPoint> &keypoints;
    Mat &descriptors;
    int nOctaveLayers;
    int firstOctave;
};

static void calcDescriptors(const std::vector<Mat> &gpyr, const std::vector<KeyPoint> &keypoints,
                            Mat &descriptors, int nOctaveLayers, int firstOctave)
{
    parallel_for_(Range(0, static_cast<int>(keypoints.size())), calcDescriptorsComputer(gpyr, keypoints, descriptors, nOctaveLayers, firstOctave));
}

int descriptorSize()
{
    return SIFT_DESCR_WIDTH * SIFT_DESCR_WIDTH * SIFT_DESCR_HIST_BINS;
}

void calDescriptorsbySIFT(InputArray _image,
                          std::vector<KeyPoint> &keypoints,
                          OutputArray _descriptors)
{
    std::cout << "SIFT!" << std::endl;
    int nOctaveLayers = 3;
    double sigma = 1.6;

    int firstOctave = -1, actualNOctaves = 0, actualNLayers = 0;
    Mat image = _image.getMat();

    if (image.empty() || image.depth() != CV_8U)
        CV_Error(Error::StsBadArg, "image is empty or has incorrect depth (!=CV_8U)");

    firstOctave = 0;
    int maxOctave = INT_MIN;
    for (size_t i = 0; i < keypoints.size(); i++)
    {
        int octave, layer;
        float scale;
        unpackOctave(keypoints[i], octave, layer, scale);
        firstOctave = std::min(firstOctave, octave);
        maxOctave = std::max(maxOctave, octave);
        actualNLayers = std::max(actualNLayers, layer - 2);
    }

    firstOctave = std::min(firstOctave, 0);
    CV_Assert(firstOctave >= -1 && actualNLayers <= nOctaveLayers);
    actualNOctaves = maxOctave - firstOctave + 1;

    Mat base = createInitialImage(image, firstOctave < 0, (float)sigma);
    std::vector<Mat> gpyr, dogpyr;
    int nOctaves = actualNOctaves > 0 ? actualNOctaves : cvRound(std::log((double)std::min(base.cols, base.rows)) / std::log(2.) - 2) - firstOctave;

    //double t, tf = getTickFrequency();
    //t = (double)getTickCount();
    buildGaussianPyramid(base, gpyr, nOctaves);
    buildDoGPyramid(gpyr, dogpyr);

    //t = (double)getTickCount() - t;
    //printf("pyramid construction time: %g\n", t*1000./tf);

    if (_descriptors.needed())
    {
        //t = (double)getTickCount();
        int dsize = descriptorSize();
        _descriptors.create((int)keypoints.size(), dsize, CV_32F);
        Mat descriptors = _descriptors.getMat();

        calcDescriptors(gpyr, keypoints, descriptors, nOctaveLayers, firstOctave);
        //t = (double)getTickCount() - t;
        //printf("descriptor extraction time: %g\n", t*1000./tf);
    }
}

bool removeNoiseByStd(std::vector<cv::Point2f> &temp_point_vec, std::vector<cv::Point2f> &current_point_vec)
{
    float x_sum = 0.0, y_sum = 0.0, x_mean, y_mean, x_std, y_std;

    for (int i = 0; i < current_point_vec.size(); i++)
    {
        x_sum += current_point_vec[i].x;
        y_sum += current_point_vec[i].y;
    }

    x_mean = x_sum / current_point_vec.size();
    y_mean = y_sum / current_point_vec.size();

    for (int i = 0; i < current_point_vec.size(); i++)
    {
        x_std += (current_point_vec[i].x - x_mean) * (current_point_vec[i].x - x_mean);
        y_std += (current_point_vec[i].y - y_mean) * (current_point_vec[i].y - y_mean);
    }

    x_std = sqrt(x_std / current_point_vec.size());
    y_std = sqrt(y_std / current_point_vec.size());

    for (int i = current_point_vec.size(); i > 0; i--)
    {
        if (fabs(current_point_vec[i].x - x_mean) > x_std || fabs(current_point_vec[i].y - y_mean) > y_std)
        {
            current_point_vec.erase(current_point_vec.begin() + i);
            temp_point_vec.erase(temp_point_vec.begin() + i);
        }
    }

    std::cout << "x_mean:" << x_mean << std::endl;
    std::cout << "y_mean:" << y_mean << std::endl;

    std::cout << "x_std:" << x_std << std::endl;
    std::cout << "y_std:" << y_std << std::endl;

    if (y_std > 200)
    {
        return true;
    }

    return true;
}