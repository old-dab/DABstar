/*
 * This file is part of the AbracaDABra project
 *
 * MIT License
 *
  * Copyright (c) 2019-2023 Petr Kopecký <xkejpi (at) gmail (dot) com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QIODevice>
#include <QObject>
#include <QWaitCondition>
#include <QMutex>
#include <QTimer>
#include <QAudioSink>
#include <QMediaDevices>

#include "audiofifo.h"

// muting
#define AUDIOOUTPUT_FADE_TIME_MS    60
// these 2 values must be aligned
#define AUDIOOUTPUT_FADE_MIN_DB    -80.0
#define AUDIOOUTPUT_FADE_MIN_LIN     0.0001

// debug switch
//#define AUDIOOUTPUT_RAW_FILE_OUT

enum class AudioOutputPlaybackState
{
  Muted   = 0,
  Playing = 1,
};


class AudioOutput : public QObject
{
  Q_OBJECT

public:
  AudioOutput(QObject * parent = nullptr)
  {
    m_devices = new QMediaDevices(this);
    connect(m_devices, &QMediaDevices::audioOutputsChanged, this, &AudioOutput::updateAudioDevices);
  }

  virtual void start(audioFifo_t * buffer) = 0;
  virtual void restart(audioFifo_t * buffer) = 0;
  virtual void stop() = 0;
  virtual void mute(bool on) = 0;
  virtual void setVolume(int value) = 0;
  virtual void setAudioDevice(const QByteArray & deviceId) = 0;

  QList<QAudioDevice> getAudioDevices()
  {
    QList<QAudioDevice> list;
    const QAudioDevice & defaultDeviceInfo = m_devices->defaultAudioOutput();
    list.append(defaultDeviceInfo);

    for (auto & deviceInfo : m_devices->audioOutputs())
    {
      if (deviceInfo != defaultDeviceInfo)
      {
        list.append(deviceInfo);
      }
    }
    return list;
  }

signals:
  void audioOutputError();
  void audioOutputRestart();
  void audioDevicesList(QList<QAudioDevice> deviceList);
  void audioDeviceChanged(const QByteArray & id);

protected:
  QMediaDevices * m_devices;
  QAudioDevice m_currentAudioDevice;

  void updateAudioDevices()
  {
    QList<QAudioDevice> list = getAudioDevices();

    emit audioDevicesList(list);

    bool currentDeviceFound = false;
    for (auto & dev : list)
    {
      if (dev.id() == m_currentAudioDevice.id())
      {
        currentDeviceFound = true;
        break;
      }
    }

    if (!currentDeviceFound)
    {   // current device no longer exists => default is used
      m_currentAudioDevice = m_devices->defaultAudioOutput();
    }
    emit audioDeviceChanged(m_currentAudioDevice.id());
  }
};

#endif // AUDIOOUTPUT_H
