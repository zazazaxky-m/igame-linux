// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cmath>
#include <complex>
#include <vector>

// Magnitude spectrum of a block of real samples. The 2N real samples are packed into an N-point
// complex FFT (even samples as real parts, odd as imaginary) and unpacked afterwards, which
// halves the work; twiddle factors and the bit-reversal order are computed once.
class RealFft
{
public:
    explicit RealFft(int size)
        : m_half(size / 2)
        , m_buf(size_t(size / 2))
        , m_twiddle(size_t(size / 4))
        , m_unpack(size_t(size / 2 + 1))
        , m_reverse(size_t(size / 2))
    {
        constexpr double pi = 3.14159265358979323846;
        for (int k = 0; k < m_half / 2; ++k)
            m_twiddle[size_t(k)] = std::polar(1.0f, float(-2 * pi * k / m_half));
        for (int k = 0; k <= m_half; ++k)
            m_unpack[size_t(k)] = std::polar(1.0f, float(-2 * pi * k / size));
        int bits = 0;
        while ((1 << bits) < m_half)
            ++bits;
        for (int i = 0; i < m_half; ++i) {
            int r = 0;
            for (int b = 0; b < bits; ++b)
                r |= ((i >> b) & 1) << (bits - 1 - b);
            m_reverse[size_t(i)] = r;
        }
    }

    int size() const { return 2 * m_half; }

    // in: size() samples (already windowed or not), out: size()/2 + 1 magnitudes.
    void magnitudes(const float *in, float *out)
    {
        for (int n = 0; n < m_half; ++n)
            m_buf[size_t(m_reverse[size_t(n)])] = {in[2 * n], in[2 * n + 1]};
        for (int len = 2; len <= m_half; len <<= 1) {
            const int step = m_half / len;
            for (int i = 0; i < m_half; i += len) {
                for (int k = 0; k < len / 2; ++k) {
                    const std::complex<float> u = m_buf[size_t(i + k)];
                    const std::complex<float> v = m_buf[size_t(i + k + len / 2)] * m_twiddle[size_t(k * step)];
                    m_buf[size_t(i + k)] = u + v;
                    m_buf[size_t(i + k + len / 2)] = u - v;
                }
            }
        }
        for (int k = 0; k <= m_half; ++k) {
            const std::complex<float> z = m_buf[size_t(k % m_half)];
            const std::complex<float> zc = std::conj(m_buf[size_t((m_half - k) % m_half)]);
            const std::complex<float> even = 0.5f * (z + zc);
            const std::complex<float> odd = std::complex<float>(0, -0.5f) * (z - zc);
            out[k] = std::sqrt(std::norm(even + m_unpack[size_t(k)] * odd)); // cheaper than std::abs (hypot)
        }
    }

private:
    int m_half;
    std::vector<std::complex<float>> m_buf;
    std::vector<std::complex<float>> m_twiddle;
    std::vector<std::complex<float>> m_unpack;
    std::vector<int> m_reverse;
};
