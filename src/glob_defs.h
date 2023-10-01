/*
 * Copyright (c) 2023 by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * DABstar is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the License, or any later version.
 *
 * DABstar is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with DABstar. If not, write to the Free Software
 * Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Keep this Qt-free as also needed for Qt-free libs

#ifndef GLOB_DEFS_H
#define GLOB_DEFS_H

#include <cstdint>
#include <complex>

using cmplx = std::complex<float>;

template<typename T> inline T conv_rad_to_deg(T iVal)
{
  return iVal * (T)(180.0 / 3.14159265358979323846);
}

template<typename T> inline T conv_deg_to_rad(T iVal)
{
  return iVal * (T)(3.14159265358979323846 / 180.0);
}

template<typename T> inline void limit_min_max(T & ioVal, T iMin, T iMax)
{
  if (ioVal < iMin)
  {
    ioVal = iMin;
  }
  else if (ioVal > iMax)
  {
    ioVal = iMax;
  }
}

template<typename T> inline void limit_symmetrically(T & ioVal, const T iLimit)
{
  if (ioVal > iLimit)
  {
    ioVal = iLimit;
  }
  else if (ioVal < -iLimit)
  {
    ioVal = -iLimit;
  }
}

template<typename T> static inline T fixround(float v)
{
  return static_cast<T>(std::roundf(v));
}

static inline bool is_indeterminate(float x)
{
  return x != x;
}

static inline bool is_infinite(float x)
{
  return x == std::numeric_limits<float>::infinity();
}

static inline float fast_abs(cmplx z)
{
#if 0
  return std::abs(z);
#else
  // this is the former jan_abs() from Jan van Katwijk, should be faster than std::abs()... but is it really? (depends on the device)
  float re = real(z);
  float im = imag(z);

  if (re < 0)
  {
    re = -re;
  }
  if (im < 0)
  {
    im = -im;
  }
  if (re > im)
  {
    return re + 0.5f * im;
  }
  else
  {
    return im + 0.5f * re;
  }
#endif
}

inline cmplx norm_to_length_one(const cmplx & iVal)
{
  return iVal / std::fabs(iVal);
}

inline cmplx cmplx_from_phase(const float iPhase)
{
  return std::exp(cmplx(0.0f, iPhase));
}

inline cmplx abs_log10_with_offset_and_phase(const cmplx & iVal)
{
  return std::log10(std::abs(iVal) + 1.0f) * std::exp(cmplx(0.0f, std::arg(iVal)));
}

inline float turn_phase_to_first_quadrant(float iPhase)
{
  // following line does not work if a cyclic turn should be detected
  // return std::atan2(std::abs(sin(iPhase)), std::abs(cos(iPhase)));
  if (iPhase < 0.0f)
  {
    iPhase += M_PI; // fmod will not work as intended with negative values
  }
  return std::fmod(iPhase, (float)M_PI_2);
}

template<typename T> inline float abs_log10_with_offset(const T iVal)
{
  return std::log10(std::abs(iVal) + 1);
}

template <typename T> inline void mean_filter(T & ioVal, T iVal, const T iAlpha)
{
  ioVal += iAlpha * (iVal - ioVal);
}

template <typename T> inline void create_blackman_window(T * opVal, int32_t iWindowWidth)
{
#if 0 // The exact(er) version place zeros at the 3rd and 4th side-lobes but result in a discontinuity at the edges and a 6 dB/oct fall-off
  constexpr double a0 = 7938.0/18608.0; // 0.426590714
  constexpr double a1 = 9240.0/18608.0; // 0.496560619
  constexpr double a2 = 1430.0/18608.0; // 0.076848667.

  for (int32_t i = 0; i < iWindowWidth; i++)
  {
    opVal[i] = static_cast<T>(a0
                            - a1 * cos((2.0 * M_PI * i) / (iWindowWidth - 1))
                            + a2 * cos((4.0 * M_PI * i) / (iWindowWidth - 1)));
  }
#else // The truncated coefficients do not null the sidelobes as well, but have an improved 18 dB/oct fall-off
  for (int32_t i = 0; i < iWindowWidth; i++)
  {
    opVal[i] = static_cast<T>(0.42
                            - 0.50 * cos((2.0 * M_PI * i) / (iWindowWidth - 1))
                            + 0.08 * cos((4.0 * M_PI * i) / (iWindowWidth - 1)));
  }
#endif
}

#endif // GLOB_DEFS_H
