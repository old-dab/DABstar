/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB
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

#include  "audiosink.h"
#include  <cstdio>
#include  <QDebug>
#include  <QComboBox>

AudioSink::AudioSink(int16_t latency) :
  _O_Buffer(8 * 32768)
{
  int32_t i;
  if (latency <= 0)
  {
    latency = 1;
  }
  this->latency = latency;
  this->CardRate = 48000;
  portAudio = false;
  writerRunning = false;
  if (Pa_Initialize() != paNoError)
  {
    fprintf(stderr, "Initializing Pa for output failed\n");
    return;
  }

  portAudio = true;

  qDebug("Hostapis: %d", Pa_GetHostApiCount());

  for (i = 0; i < Pa_GetHostApiCount(); i++)
    qDebug("  Api %d is %s", i, Pa_GetHostApiInfo(i)->name);

  numofDevices = Pa_GetDeviceCount();
  outTable.resize(numofDevices + 1);
  for (i = 0; i < numofDevices; i++)
  {
    outTable[i] = -1;
  }
  ostream = nullptr;
  theMissed = 0;
  totalSamples = 1;
}

AudioSink::~AudioSink()
{
  if ((ostream != nullptr) && !Pa_IsStreamStopped(ostream))
  {
    paCallbackReturn = paAbort;
    (void)Pa_AbortStream(ostream);
    while (!Pa_IsStreamStopped(ostream))
    {
      Pa_Sleep(1);
    }
    writerRunning = false;
  }

  if (ostream != nullptr)
  {
    Pa_CloseStream(ostream);
  }

  if (portAudio)
  {
    Pa_Terminate();
  }
}

bool AudioSink::selectDevice(int16_t idx)
{
  PaError err;
  int16_t outputDevice;

  if (idx == 0)
  {
    return false;
  }

  outputDevice = outTable[idx];
  if (!isValidDevice(outputDevice))
  {
    fprintf(stderr, "invalid device (%d) selected\n", outputDevice);
    return false;
  }

  if ((ostream != nullptr) && !Pa_IsStreamStopped(ostream))
  {
    paCallbackReturn = paAbort;
    (void)Pa_AbortStream(ostream);
    while (!Pa_IsStreamStopped(ostream))
    {
      Pa_Sleep(1);
    }
    writerRunning = false;
  }

  if (ostream != nullptr)
  {
    Pa_CloseStream(ostream);
  }

  outputParameters.device = outputDevice;
  outputParameters.channelCount = 2;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputDevice)->defaultHighOutputLatency;
  bufSize = (int)((float)outputParameters.suggestedLatency * latency);
  //
  //	A small buffer causes more callback invocations, sometimes
  //	causing underflows and intermittent output.
  //	buffersize is

  //	bufSize	= latency * 512;

  outputParameters.hostApiSpecificStreamInfo = nullptr;
  //
  fprintf(stdout, "Suggested size for outputbuffer = %d\n", bufSize);
  err = Pa_OpenStream(&ostream, nullptr, &outputParameters, CardRate, bufSize, 0, this->paCallback_o, this);

  if (err != paNoError)
  {
    qDebug("Open ostream error\n");
    return false;
  }
  fprintf(stdout, "stream opened\n");
  paCallbackReturn = paContinue;
  err = Pa_StartStream(ostream);
  if (err != paNoError)
  {
    qDebug("Open startstream error\n");
    return false;
  }
  fprintf(stdout, "stream started\n");
  writerRunning = true;
  return true;
}

void AudioSink::restart()
{
  PaError err;

  if (!Pa_IsStreamStopped(ostream))
  {
    return;
  }

  _O_Buffer.flush_ring_buffer();
  totalSamples = 1;
  paCallbackReturn = paContinue;
  err = Pa_StartStream(ostream);
  if (err == paNoError)
  {
    writerRunning = true;
  }
}

void AudioSink::stop()
{
  if (Pa_IsStreamStopped(ostream))
  {
    return;
  }

  //	paCallbackReturn	= paAbort;
  (void)Pa_StopStream(ostream);
  while (!Pa_IsStreamStopped(ostream))
  {
    Pa_Sleep(1);
  }
  writerRunning = false;
  _O_Buffer.flush_ring_buffer();
}

//
//	helper
bool AudioSink::OutputrateIsSupported(int16_t device, int32_t Rate)
{
  PaStreamParameters * outputParameters = (PaStreamParameters *)alloca (sizeof(PaStreamParameters));

  outputParameters->device = device;
  outputParameters->channelCount = 2;  /* I and Q	*/
  outputParameters->sampleFormat = paFloat32;
  outputParameters->suggestedLatency = 0;
  outputParameters->hostApiSpecificStreamInfo = nullptr;

  return Pa_IsFormatSupported(nullptr, outputParameters, Rate) == paFormatIsSupported;
}

/*
 * 	... and the callback
 */

int AudioSink::paCallback_o(const void * inputBuffer, void * outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags, void * userData)
{
  RingBuffer<float> * outB;
  float * outp = (float *)outputBuffer;
  AudioSink * ud = reinterpret_cast <AudioSink *>(userData);
  uint32_t actualSize;
  uint32_t i;
  (void)statusFlags;
  (void)inputBuffer;
  (void)timeInfo;
  if (ud->paCallbackReturn == paContinue)
  {
    outB = &((reinterpret_cast < AudioSink *> (userData))->_O_Buffer);
    actualSize = outB->get_data_from_ring_buffer(outp, 2 * framesPerBuffer);
    ud->theMissed += 2 * framesPerBuffer - actualSize;
    ud->totalSamples += 2 * framesPerBuffer;
    for (i = actualSize; i < 2 * framesPerBuffer; i++)
    {
      outp[i] = 0;
    }
  }

  return ud->paCallbackReturn;
}

bool AudioSink::hasMissed()
{
  return true;
}

int32_t AudioSink::missed()
{
  if (totalSamples == 0)
  {
    fprintf(stderr, "HELP\n");
  }
  int32_t h = 100 - (theMissed * 100) / totalSamples;
  theMissed = 0;
  totalSamples = 1;
  return h;
}

void AudioSink::audioOutput(float * b, int32_t amount)
{
  if (_O_Buffer.get_ring_buffer_write_available() < 2 * amount)
  {
    fprintf(stderr, "%d\n", 2 * amount - _O_Buffer.get_ring_buffer_read_available());
  }
  _O_Buffer.put_data_into_ring_buffer(b, 2 * amount);
}

QString AudioSink::outputChannelwithRate(int16_t ch, int32_t rate)
{
  const PaDeviceInfo * deviceInfo;
  QString name = QString("");

  if ((ch < 0) || (ch >= numofDevices))
  {
    return name;
  }

  deviceInfo = Pa_GetDeviceInfo(ch);
  if (deviceInfo == nullptr)
  {
    return name;
  }
  if (deviceInfo->maxOutputChannels <= 0)
  {
    return name;
  }

  if (OutputrateIsSupported(ch, rate))
  {
    name = QString(deviceInfo->name);
  }
  return name;
}

int16_t AudioSink::invalidDevice()
{
  return numofDevices + 128;
}

bool AudioSink::isValidDevice(int16_t dev)
{
  return 0 <= dev && dev < numofDevices;
}

bool AudioSink::selectDefaultDevice()
{
  return selectDevice(Pa_GetDefaultOutputDevice());
}

int32_t AudioSink::cardRate()
{
  return 48000;
}

bool AudioSink::setupChannels(QComboBox * streamOutSelector)
{
  uint16_t ocnt = 1;
  uint16_t i;

  for (i = 0; i < numofDevices; i++)
  {
    const QString so = outputChannelwithRate(i, 48000);
    qDebug("Investigating Device %d", i);

    if (so != QString(""))
    {
      streamOutSelector->insertItem(ocnt, so, QVariant(i));
      outTable[ocnt] = i;
      qDebug(" (output):item %d -> stream %d (%s)", ocnt, i, so.toUtf8().data());
      ocnt++;
    }
  }

  qDebug() << "added items to combobox";
  return ocnt > 1;
}

//
int16_t AudioSink::numberofDevices()
{
  return numofDevices;
}
