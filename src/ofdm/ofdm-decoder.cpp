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
 *    This file is part of Qt-DAB
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
 *
 *	Once the bits are "in", interpretation and manipulation
 *	should reconstruct the data blocks.
 *	Ofdm_decoder is called for Block_0 and the FIC blocks,
 *	its invocation results in 2 * Tu bits
 */
#include "ofdm-decoder.h"
#include "phasetable.h"
#include "radio.h"
#include <vector>

/**
 *	\brief OfdmDecoder
 *	The class OfdmDecoder is
 *	taking the data from the ofdmProcessor class in, and
 *	will extract the Tu samples, do an FFT and extract the
 *	carriers and map them on (soft) bits
 */
OfdmDecoder::OfdmDecoder(RadioInterface * ipMr, uint8_t iDabMode, RingBuffer<cmplx> * ipIqBuffer, RingBuffer<float> * ipCarrBuffer) :
  mpRadioInterface(ipMr),
  mDabPar(DabParams(iDabMode).get_dab_par()),
  mFreqInterleaver(iDabMode),
  mFftHandler(mDabPar.T_u, false),
  mpIqBuffer(ipIqBuffer),
  mpCarrBuffer(ipCarrBuffer)
{
  connect(this, &OfdmDecoder::signal_slot_show_iq, mpRadioInterface, &RadioInterface::slot_show_iq);
  connect(this, &OfdmDecoder::signal_show_mod_quality, mpRadioInterface, &RadioInterface::slot_show_mod_quality_data);

  mMeanNullLevelWithTII.resize(mDabPar.T_u);
  mMeanNullPowerWithoutTII.resize(mDabPar.T_u);
  mPhaseReference.resize(mDabPar.T_u);
  mFftBuffer.resize(mDabPar.T_u);
  mIqVector.resize(mDabPar.K);
  mCarrVector.resize(mDabPar.K);
  mStdDevSqPhaseVector.resize(mDabPar.K);
  mMeanAbsPhaseVector.resize(mDabPar.K);
  mMeanPhaseVector.resize(mDabPar.K);
  mMeanPowerVector.resize(mDabPar.K);

  reset();
}

void OfdmDecoder::reset()
{
  std::fill(mStdDevSqPhaseVector.begin(), mStdDevSqPhaseVector.end(), 0.0f);
  std::fill(mMeanAbsPhaseVector.begin(), mMeanAbsPhaseVector.end(), (float)M_PI_4);
  std::fill(mMeanPhaseVector.begin(), mMeanPhaseVector.end(), 0.0f);
  std::fill(mMeanPowerVector.begin(), mMeanPowerVector.end(), 0.0f);
  std::fill(mMeanNullLevelWithTII.begin(), mMeanNullLevelWithTII.end(), 0.0f);
  std::fill(mMeanNullPowerWithoutTII.begin(), mMeanNullPowerWithoutTII.end(), 0.0f);

  mMeanPowerOvrAll = 1.0f;
  mAvgAbsNullLevelWithTIIMax = 0.0f;
  mAvgAbsNullLevelWithTIIMin = 0.0f;
  mAvgAbsNullLevelWithTIIGain = 0.0f;
}

void OfdmDecoder::store_null_with_tii_block(const std::vector<cmplx> & iV) // with TII information
{
  if (mPlotType != ECarrierPlotType::NULLTII)
  {
    return;
  }

  memcpy(mFftBuffer.data(), &(iV[mDabPar.T_g]), mDabPar.T_u * sizeof(cmplx));

  mFftHandler.fft(mFftBuffer);

  constexpr float ALPHA = 0.20f;
  float max = -1e100;
  float min =  1e100;

  for (int32_t idx = 0; idx < mDabPar.T_u; ++idx)
  {
    const float level = std::abs(mFftBuffer[idx]);

    float & meanNullLevelWithTIIRef = mMeanNullLevelWithTII[idx];
    mean_filter(meanNullLevelWithTIIRef, level, ALPHA);

    if (level < min) min = meanNullLevelWithTIIRef;
    if (level > max) max = meanNullLevelWithTIIRef;
  }

  mean_filter(mAvgAbsNullLevelWithTIIMin, min, ALPHA);
  mean_filter(mAvgAbsNullLevelWithTIIMax, max, ALPHA);

  assert(mAvgAbsNullLevelWithTIIMax > mAvgAbsNullLevelWithTIIMin);

  mAvgAbsNullLevelWithTIIGain = 100.0f / (mAvgAbsNullLevelWithTIIMax - mAvgAbsNullLevelWithTIIMin);
}

void OfdmDecoder::store_null_without_tii_block(const std::vector<cmplx> & iV) // with TII information
{
  memcpy(mFftBuffer.data(), &(iV[mDabPar.T_g]), mDabPar.T_u * sizeof(cmplx));

  mFftHandler.fft(mFftBuffer);

  constexpr float ALPHA = 0.1f;

  for (int32_t idx = 0; idx < mDabPar.T_u; ++idx)
  {
    const float level = std::abs(mFftBuffer[idx]);
    mean_filter(mMeanNullPowerWithoutTII[idx], level * level, ALPHA);
  }
}

/**
 */
void OfdmDecoder::process_reference_block_0(std::vector<cmplx> buffer) // copy is intended as used as fft buffer
{
  mFftHandler.fft(buffer);
  /**
   *	we are now in the frequency domain, and we keep the carriers
   *	as coming from the FFT as phase reference.
   */

  memcpy(mPhaseReference.data(), buffer.data(), mDabPar.T_u * sizeof(cmplx));
}

/**
 *	for the other blocks of data, the first step is to go from
 *	time to frequency domain, to get the carriers.
 *	we distinguish between FIC blocks and other blocks,
 *	only to spare a test. The mapping code is the same
 */

void OfdmDecoder::decode(const std::vector<cmplx> & iV, uint16_t iCurOfdmSymbIdx, float iPhaseCorr, std::vector<int16_t> & oBits)
{
  memcpy(mFftBuffer.data(), &(iV[mDabPar.T_g]), mDabPar.T_u * sizeof(cmplx));

  mFftHandler.fft(mFftBuffer);

  const cmplx rotator = std::exp(cmplx(0.0f, -iPhaseCorr)); // fine correction of phase which can't be done in the time domain

  /**
   *	a little optimization: we do not interchange the
   *	positive/negative frequencies to their right positions.
   *	The de-interleaving understands this
   *
   *	Note that from here on, we are only interested in the
   *	"carriers", the useful carriers of the FFT output
   */
  for (int16_t nomCarrIdx = 0; nomCarrIdx < mDabPar.K; ++nomCarrIdx)
  {
    int16_t fftIdx = mFreqInterleaver.map_k_to_fft_bin(nomCarrIdx);
    int16_t realCarrRelIdx = fftIdx;

    if (fftIdx < 0)
    {
      realCarrRelIdx += mDabPar.K / 2;
      fftIdx += mDabPar.T_u;
    }
    else // > 0 (there is no 0)
    {
      realCarrRelIdx += mDabPar.K / 2 - 1;
    }

    const int16_t dataVecCarrIdx = (mShowNomCarrier ? nomCarrIdx : realCarrRelIdx);

    /**
     *	decoding is computing the phase difference between
     *	carriers with the same index in subsequent blocks.
     *	The carrier of a block is the reference for the carrier
     *	on the same position in the next block
     */
#ifndef OTHER_OFDM_DECODING_STYLE
    constexpr float ALPHA = 0.005f;

    cmplx fftBin = mFftBuffer[fftIdx] * norm_to_length_one(conj(mPhaseReference[fftIdx])); // PI/4-DQPSK demodulation
    fftBin *= rotator; // fine correction of phase which can't be done in the time domain

    // get mean of absolute phase for each bin
    const float fftBinPhase = std::arg(fftBin);
    const float fftBinAbsPhase = turn_phase_to_first_quadrant(fftBinPhase);
    float & meanAbsPhasePerBinRef = mMeanAbsPhaseVector[nomCarrIdx];
    mean_filter(meanAbsPhasePerBinRef, fftBinAbsPhase, ALPHA);

    // get standard deviation of absolute phase for each bin
    const float curStdDevDiff = (fftBinAbsPhase - meanAbsPhasePerBinRef);
    const float curStdDevSq = curStdDevDiff * curStdDevDiff;
    float & stdDevSqRef =  mStdDevSqPhaseVector[nomCarrIdx];
    mean_filter(stdDevSqRef, curStdDevSq, ALPHA);
    const float avgStdDev = sqrt(stdDevSqRef); // this value should be only between 0 (no noise) and M_PI_4 (heavy noise etc.)

    // finally calculate (and limit) a soft bit weight from the standard deviation for each bin
    float weight = 127.0f * ((float)M_PI_4 - avgStdDev) / (float)M_PI_4;
    limit_min_max(weight, 2.0f, 127.0f);  // at least 2 as viterbi shows problems with only 1

    // calculate the mean of the vector length of each bin to equalize the IQ diagram (simply looks nicer)
    const float fftBinAbsLevel = std::abs(fftBin);
    const float fftBinPower = fftBinAbsLevel * fftBinAbsLevel;

    float & meanPowerPerBinRef = mMeanPowerVector[nomCarrIdx];
    mean_filter(meanPowerPerBinRef, fftBinPower, ALPHA);
    mean_filter(mMeanPowerOvrAll, fftBinPower, ALPHA / 1536.0f);
    mIqVector[nomCarrIdx] = fftBin / sqrt(meanPowerPerBinRef);

    switch (mPlotType)
    {
    case ECarrierPlotType::FOURQUADPHASE:
      mCarrVector[dataVecCarrIdx] = conv_rad_to_deg(std::arg(fftBin));
      break;
    case ECarrierPlotType::MODQUAL:
      mCarrVector[dataVecCarrIdx] = 100.0f / 127.0f * weight; // weight is shown from 0..100%
      break;
    case ECarrierPlotType::STDDEV:
      mCarrVector[dataVecCarrIdx] = conv_rad_to_deg(avgStdDev);
      break;
    case ECarrierPlotType::RELPOWER:
      mCarrVector[dataVecCarrIdx] = 10.0f * std::log10(meanPowerPerBinRef / mMeanPowerOvrAll);
      break;
    case ECarrierPlotType::MEANABSPHASE:
      mCarrVector[dataVecCarrIdx] = conv_rad_to_deg(meanAbsPhasePerBinRef);
      break;
//    case ECarrierPlotType::MEANPHASE:
//    {
//      float & meanPhasePerBinRef = mMeanPhaseVector[nomCarrIdx];
//      mean_filter(meanPhasePerBinRef, fftBinPhase, 0.1f * ALPHA);
//      mCarrVector[dataVecCarrIdx] = conv_rad_to_deg(meanPhasePerBinRef);
//      break;
//    }
    case ECarrierPlotType::NULLTII:
      mCarrVector[dataVecCarrIdx] = mAvgAbsNullLevelWithTIIGain * (mMeanNullLevelWithTII[fftIdx] - mAvgAbsNullLevelWithTIIMin); // TII is shown from 0..100%
      break;
    case ECarrierPlotType::SNR:
      mCarrVector[dataVecCarrIdx] = 10.0f * std::log10(meanPowerPerBinRef / mMeanNullPowerWithoutTII[fftIdx]);
      break;
    }

    const int16_t hardBitReal = (real(fftBin) < 0.0f ? 1 : -1);
    const int16_t hardBitImag = (imag(fftBin) < 0.0f ? 1 : -1);

    oBits[0         + nomCarrIdx] = (int16_t)(hardBitReal * weight);
    oBits[mDabPar.K + nomCarrIdx] = (int16_t)(hardBitImag * weight);
#else
    cmplx r1 = mFftBuffer[fftIdx] * conj(mPhaseReference[fftIdx]);
    //r1 *= rotator; // fine correction of phase which can't be done in the time domain
    const float ab1 = abs(r1);
    mean_filter(mAvgAbsLevelOvrAll, ab1, 0.005f / 1536.0f);
    mIqVector[nomCarrIdx] = r1 / mAvgAbsLevelOvrAll;
    mCarrVector[dataVecCarrIdx] = conv_rad_to_deg(std::arg(r1));

    // split the real and the imaginary part and scale it we make the bits into softbits in the range -127 .. 127 (+/- 255?)
    // tomneda: is that kind of soft creation ok?
    oBits[0         + nomCarrIdx] = (int16_t)(-(real(r1) * 255.0f) / ab1);
    oBits[mDabPar.K + nomCarrIdx] = (int16_t)(-(imag(r1) * 255.0f) / ab1);
#endif
  } // for (nomCarrIdx...


  //	From time to time we show the constellation of the current symbol
  if (++mShowCntIqScope > mDabPar.L && iCurOfdmSymbIdx == mNextShownOfdmSymbIdx)
  {
    mpIqBuffer->putDataIntoBuffer(mIqVector.data(), mDabPar.K);
    mpCarrBuffer->putDataIntoBuffer(mCarrVector.data(), mDabPar.K);
    emit signal_slot_show_iq(mDabPar.K, 1.0f /*mGain / TOP_VAL*/);
    mShowCntIqScope = 0;
  }

  if (++mShowCntStatistics > 5 * mDabPar.L && iCurOfdmSymbIdx == mNextShownOfdmSymbIdx)
  {
    mQD.CurOfdmSymbolNo = iCurOfdmSymbIdx + 1; // as "idx" goes from 0...(L-1)
    mQD.StdDeviation = _compute_mod_quality(mIqVector);
    mQD.TimeOffset = _compute_time_offset(mFftBuffer, mPhaseReference);
    mQD.FreqOffset = _compute_frequency_offset(mFftBuffer, mPhaseReference);
    mQD.PhaseCorr = -conv_rad_to_deg(iPhaseCorr);
    mQD.SNR = 10.0f * std::log10(mMeanPowerOvrAll / _compute_noise_Power());
    emit signal_show_mod_quality(&mQD);
    mShowCntStatistics = 0;
    mNextShownOfdmSymbIdx = (mNextShownOfdmSymbIdx + 1) % mDabPar.L;
    if (mNextShownOfdmSymbIdx == 0) mNextShownOfdmSymbIdx = 1; // as iCurSymbolNo can never be zero here
  }

  memcpy(mPhaseReference.data(), mFftBuffer.data(), mDabPar.T_u * sizeof(cmplx));
}

float OfdmDecoder::_compute_mod_quality(const std::vector<cmplx> & v) const
{
  /*
   * Since we do not equalize, we have a kind of "fake" reference point.
   * The key parameter here is the phase offset, so we compute the std.
   * deviation of the phases rather than the computation from the Modulation
   * Error Ratio as specified in Tr 101 290
   */

  constexpr cmplx rotator = cmplx(1.0f, -1.0f); // this is the reference phase shift to get the phase zero degree
  float squareVal = 0;

  for (int i = 0; i < mDabPar.K; i++)
  {
    float x1 = arg(cmplx(abs(real(v[i])), abs(imag(v[i]))) * rotator); // map to top right section and shift phase to zero (nominal)
    squareVal += x1 * x1;
  }

  return sqrtf(squareVal / (float)mDabPar.K) / (float)M_PI * 180.0f; // in degree
}

/*
 * While DAB symbols do not carry pilots, it is known that
 * arg (carrier [i, j] * conj (carrier [i + 1, j])
 * should be K * M_PI / 4,  (k in {1, 3, 5, 7}) so basically
 * carriers in decoded symbols can be used as if they were pilots
 * so, with that in mind we experiment with formula 5.39
 * and 5.40 from "OFDM Baseband Receiver Design for Wireless
 * Communications (Chiueh and Tsai)"
 */
float OfdmDecoder::_compute_time_offset(const std::vector<cmplx> & r, const std::vector<cmplx> & v) const
{
  cmplx sum = cmplx(0, 0);

  for (int i = -mDabPar.K / 2; i < mDabPar.K / 2; i += 6)
  {
    int index_1 = i < 0 ? i + mDabPar.T_u : i;
    int index_2 = (i + 1) < 0 ? (i + 1) + mDabPar.T_u : (i + 1);

    cmplx s = r[index_1] * conj(v[index_2]);

    s = cmplx(abs(real(s)), abs(imag(s)));
    const cmplx leftTerm = s * conj(cmplx(fabs(s) / sqrtf(2), fabs(s) / sqrtf(2)));

    s = r[index_2] * conj(v[index_2]);
    s = cmplx(abs(real(s)), abs(imag(s)));
    const cmplx rightTerm = s * conj(cmplx(fabs(s) / sqrtf(2), fabs(s) / sqrtf(2)));

    sum += conj(leftTerm) * rightTerm;
  }

  return arg(sum);
}

float OfdmDecoder::_compute_frequency_offset(const std::vector<cmplx> & r, const std::vector<cmplx> & c) const
{
  cmplx theta = cmplx(0, 0);

  for (int i = -mDabPar.K / 2; i < mDabPar.K / 2; i += 6)
  {
    int index = i < 0 ? i + mDabPar.T_u : i;
    cmplx val = r[index] * conj(c[index]);
    val = cmplx(abs(real(val)), abs(imag(val)));
    theta += val;
  }
  theta *= cmplx(1, -1);

  return arg(theta) / (2.0f * (float)M_PI) * 2048000.0f / (float)mDabPar.T_u; // TODO: hard coded 2048000?
}

float OfdmDecoder::_compute_clock_offset(const cmplx * r, const cmplx * v) const
{
  float offsa = 0;
  int offsb = 0;

  for (int i = -mDabPar.K / 2; i < mDabPar.K / 2; i += 6)
  {
    int index = i < 0 ? (i + mDabPar.T_u) : i;
    int index_2 = i + mDabPar.K / 2;
    cmplx a1 = cmplx(abs(real(r[index])), abs(imag(r[index])));
    cmplx a2 = cmplx(abs(real(v[index])), abs(imag(v[index])));
    float s = abs(arg(a1 * conj(a2)));
    offsa += (float)index * s;
    offsb += index_2 * index_2;
  }

  return offsa / (2.0f * (float)M_PI * (float)mDabPar.T_s / (float)mDabPar.T_u * (float)offsb);
}

float OfdmDecoder::_compute_noise_Power() const
{
  float sumNoise = 0.0f;
  for (const auto & v : mMeanNullPowerWithoutTII)
  {
    sumNoise += v; // the component already squared
  }
  return sumNoise / (float)mMeanNullPowerWithoutTII.size();
}

void OfdmDecoder::slot_select_carrier_plot_type(ECarrierPlotType iPlotType)
{
  mPlotType = iPlotType;
}

void OfdmDecoder::slot_show_nominal_carrier(bool iShowNominalCarrier)
{
  mShowNomCarrier = iShowNominalCarrier;
}

