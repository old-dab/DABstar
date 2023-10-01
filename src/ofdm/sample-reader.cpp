/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2013 .. 2017
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
#include  "sample-reader.h"
#include  "radio.h"
#include  <time.h>

static inline int16_t value_for_bit_pos(int16_t b)
{
  assert(b > 0);
  int16_t res = 1;
  while (--b > 0)
  {
    res <<= 1;
  }
  return res;
}

SampleReader::SampleReader(const RadioInterface * mr, deviceHandler * iTheRig, RingBuffer<cmplx> * iSpectrumBuffer) :
  theRig(iTheRig),
  spectrumBuffer(iSpectrumBuffer),
  oneSampleBuffer(1)
{
  localBuffer.resize(bufferSize);
  dumpfilePointer.store(nullptr);
  dumpScale = value_for_bit_pos(theRig->bitDepth());
  running.store(true);

  for (int i = 0; i < INPUT_RATE; i++)
  {
    oscillatorTable[i] = cmplx((float)cos(2.0 * M_PI * i / INPUT_RATE),
                               (float)sin(2.0 * M_PI * i / INPUT_RATE));
  }

  connect(this, &SampleReader::signal_show_spectrum, mr, &RadioInterface::slot_show_spectrum);
  //connect(this, &SampleReader::signal_show_corrector, mr, &RadioInterface::slot_set_corrector_display);
}

void SampleReader::setRunning(bool b)
{
  running.store(b);
}

float SampleReader::get_sLevel() const
{
  return sLevel;
}

cmplx SampleReader::getSample(int32_t phaseOffset)
{
  getSamples(oneSampleBuffer, 0, 1, phaseOffset);
  return oneSampleBuffer[0];
}

void SampleReader::getSamples(std::vector<cmplx> & oV, const int32_t iStartIdx, int32_t iNoSamples, const int32_t iFreqOffsetBBHz)
{
  assert((signed)oV.size() >= iStartIdx + iNoSamples);
  
  std::vector<cmplx> buffer(iNoSamples);

  if (!running.load()) throw 21;

  if (iNoSamples > bufferContent)
  {
    _wait_for_sample_buffer_filled(iNoSamples);
  }

  if (!running.load()) throw 20;

  iNoSamples = theRig->getSamples(buffer.data(), iNoSamples);
  bufferContent -= iNoSamples;

  if (dumpfilePointer.load() != nullptr)
  {
    for (int32_t i = 0; i < iNoSamples; i++)
    {
      _dump_sample_to_file(oV[i]);
    }
  }

  //	OK, we have samples!!
  //	first: adjust frequency. We need Hz accuracy
  for (int32_t i = 0; i < iNoSamples; i++)
  {
    currentPhase -= iFreqOffsetBBHz;
    //
    //	Note that "phase" itself might be negative
    currentPhase = (currentPhase + INPUT_RATE) % INPUT_RATE;
    if (localCounter < bufferSize)
    {
      localBuffer[localCounter] = oV[i];
      ++localCounter;
    }
    oV[iStartIdx + i] = buffer[i] * oscillatorTable[currentPhase];
    mean_filter(sLevel, fast_abs(oV[i]), 0.00001f);
  }

  sampleCount += iNoSamples;

  if (sampleCount > INPUT_RATE / 4)
  {
    //emit signal_show_corrector(iFreqOffsetBBHz);
    if (spectrumBuffer != nullptr)
    {
      spectrumBuffer->putDataIntoBuffer(localBuffer.data(), bufferSize);
      emit signal_show_spectrum(bufferSize);
    }
    localCounter = 0;
    sampleCount = 0;
  }
}

void SampleReader::_wait_for_sample_buffer_filled(int32_t n)
{
  bufferContent = theRig->Samples();
  while ((bufferContent < n) && running.load())
  {
    constexpr timespec ts{ 0, 10'000L };
    while (nanosleep(&ts, nullptr)); // while is very likely obsolete
    bufferContent = theRig->Samples();
  }
}

void SampleReader::_dump_sample_to_file(const cmplx & v)
{
  dumpBuffer[2 * dumpIndex + 0] = fixround<int16_t>(real(v) * dumpScale);
  dumpBuffer[2 * dumpIndex + 1] = fixround<int16_t>(imag(v) * dumpScale);

  if (++dumpIndex >= DUMPSIZE / 2)
  {
    sf_writef_short(dumpfilePointer.load(), dumpBuffer.data(), dumpIndex);
    dumpIndex = 0;
  }
}

void SampleReader::startDumping(SNDFILE * f)
{
  dumpfilePointer.store(f);
}

void SampleReader::stopDumping()
{
  dumpfilePointer.store(nullptr);
}

