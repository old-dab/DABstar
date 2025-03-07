/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2015 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB program
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "fft-handler.h"
#include "fft-complex.h"

FftHandler::FftHandler(const int iSize, const bool iDir)
{
  size = iSize;
  dir = iDir;
#ifdef  __KISS_FFT__
  fftVector_in = new kiss_fft_cpx[size];
  fftVector_out = new kiss_fft_cpx[size];
  plan = kiss_fft_alloc(size, dir, nullptr, nullptr);
#elif  __FFTW3__
  fftCmplxVector = (cmplx*)fftwf_malloc(sizeof(cmplx) * size);
  fftFloatVector = (float*)fftwf_malloc(sizeof(float) * size);
  planCmplx = fftwf_plan_dft_1d(size,
                                reinterpret_cast<fftwf_complex*>(fftCmplxVector),
                                reinterpret_cast<fftwf_complex*>(fftCmplxVector),
                                (dir == 0 ? FFTW_FORWARD : FFTW_BACKWARD),
                                FFTW_ESTIMATE);
  planFloat = fftwf_plan_dft_r2c_1d(size,
                                    fftFloatVector,
                                    reinterpret_cast<fftwf_complex*>(fftCmplxVector),
                                    FFTW_ESTIMATE);
#endif
}

FftHandler::~FftHandler()
{
#ifdef  __KISS_FFT__
  delete fftVector_in;
  delete fftVector_out;
#elif  __FFTW3__
  fftwf_destroy_plan(planCmplx);
  fftwf_free(fftCmplxVector);
  fftwf_destroy_plan(planFloat);
  fftwf_free(fftFloatVector);
#endif
}

void FftHandler::fft(std::vector<cmplx> & ioV) const
{
#ifdef  __KISS_FFT__
  for (int i = 0; i < size; i++)
  {
    fftVector_in[i].r = real(ioV[i]);
    fftVector_in[i].i = imag(ioV[i]);
  }

  kiss_fft(plan, fftVector_in, fftVector_out);

  for (int i = 0; i < size; i++)
  {
    ioV[i] = cmplx(fftVector_out[i].r, fftVector_out[i].i);
  }
#elif __FFTW3__
  for (int i = 0; i < size; i++)
  {
    fftCmplxVector[i] = ioV[i];
  }

  fftwf_execute(planCmplx);

  for (int i = 0; i < size; i++)
  {
    ioV[i] = fftCmplxVector[i];
  }
#else
  Fft_transform(ioV.data (), size, dir);
#endif
}

void FftHandler::fft(cmplx * const ioV) const
{
#ifdef  __KISS_FFT__
  for (int i = 0; i < size; i++)
  {
    fftVector_in[i].r = real(ioV[i]);
    fftVector_in[i].i = imag(ioV[i]);
  }

  kiss_fft(plan, fftVector_in, fftVector_out);

  for (int i = 0; i < size; i++)
  {
    ioV[i] = cmplx(fftVector_out[i].r, fftVector_out[i].i);
  }
#elif  __FFTW3__
  for (int i = 0; i < size; i++)
  {
    fftCmplxVector[i] = ioV[i];
  }

  fftwf_execute(planCmplx);

  for (int i = 0; i < size; i++)
  {
    ioV[i] = fftCmplxVector[i];
  }
#else
  Fft_transform(ioV, size, dir);
#endif
}

void FftHandler::fft(float * const ioV) const
{
#ifdef  __KISS_FFT__
  for (int i = 0; i < size; i++)  
  {
    fftVector_in[i].r = ioV[i];
    fftVector_in[i].i = 0;
  }

  kiss_fft(plan, fftVector_in, fftVector_out);

  for (int i = 0; i < size / 2; i++) // only the first half is used
  {
    ioV[i] = std::abs(cmplx(fftVector_out[i].r, fftVector_out[i].i));
  }
#elif  __FFTW3__
  assert(dir == 0); // only forward FFT supported here

  for (int i = 0; i < size; i++)
  {
    fftFloatVector[i] = ioV[i];
  }

  fftwf_execute(planFloat);

  for (int i = 0; i < size / 2; i++) // only the first half is used
  {
    ioV[i] = std::abs(fftCmplxVector[i]);
  }
#else
  auto * const buffer = make_vla(cmplx, size);

  for (int i = 0; i < size; i++)
  {
    buffer[i] = ioV[i]; // copy only too real part
  }

  Fft_transform(buffer, size, dir);

  for (int i = 0; i < size / 2; i++) // only the first half is used
  {
    ioV[i] = std::abs(buffer[i]);
  }
#endif
}
