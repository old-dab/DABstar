/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2020
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

#ifndef  SDRPLAY_HANDLER_V3_H
#define  SDRPLAY_HANDLER_V3_H

#include  <QThread>
#include  <QFrame>
#include  <QSettings>
#include  <QSemaphore>
#include  <atomic>
#include  <cstdio>
#include  <queue>
#include  "dab-constants.h"
#include  "ringbuffer.h"
#include  "device-handler.h"
#include  "ui_sdrplay-widget-v3.h"
#include  <sdrplay_api.h>

class Rsp_device;

class generalCommand;

class XmlFileWriter;

#ifdef __MINGW32__
#include "dlfcn.h"
#define GETPROCADDRESS  GetProcAddress
#else
#define GETPROCADDRESS  dlsym
#endif

class SdrPlayHandler_v3 final : public QThread, public IDeviceHandler, public Ui_sdrplayWidget_v3
{
Q_OBJECT
public:
  SdrPlayHandler_v3(QSettings *, const QString &);
  ~SdrPlayHandler_v3() override;

  void setVFOFrequency(int32_t) override;
  int32_t getVFOFrequency() override;
  //int32_t defaultFrequency();

  bool restartReader(int32_t) override;
  void stopReader() override;
  int32_t getSamples(cmplx *, int32_t) override;
  int32_t Samples() override;
  void resetBuffer() override;
  int16_t bitDepth() override;
  void show() override;
  void hide() override;
  bool isHidden() override;
  QString deviceName() override;
  bool isFileInput() override;

  void update_PowerOverload(sdrplay_api_EventParamsT * params);
  RingBuffer<std::complex<int16_t>> _I_Buffer;
  std::atomic<bool> receiverRuns;
  int theGain;
  sdrplay_api_CallbackFnsT cbFns;

private:
  QFrame myFrame;
public:
  sdrplay_api_Open_t sdrplay_api_Open;
  sdrplay_api_Close_t sdrplay_api_Close;
  sdrplay_api_ApiVersion_t sdrplay_api_ApiVersion;
  sdrplay_api_LockDeviceApi_t sdrplay_api_LockDeviceApi;
  sdrplay_api_UnlockDeviceApi_t sdrplay_api_UnlockDeviceApi;
  sdrplay_api_GetDevices_t sdrplay_api_GetDevices;
  sdrplay_api_SelectDevice_t sdrplay_api_SelectDevice;
  sdrplay_api_ReleaseDevice_t sdrplay_api_ReleaseDevice;
  sdrplay_api_GetErrorString_t sdrplay_api_GetErrorString;
  sdrplay_api_GetLastError_t sdrplay_api_GetLastError;
  sdrplay_api_DebugEnable_t sdrplay_api_DebugEnable;
  sdrplay_api_GetDeviceParams_t sdrplay_api_GetDeviceParams;
  sdrplay_api_Init_t sdrplay_api_Init;
  sdrplay_api_Uninit_t sdrplay_api_Uninit;
  sdrplay_api_Update_t sdrplay_api_Update;

  sdrplay_api_DeviceT * chosenDevice;
  Rsp_device * theRsp;

  int inputRate;
  std::atomic<bool> failFlag;
  std::atomic<bool> successFlag;
  std::atomic<bool> threadRuns;
  void run() override;
  bool messageHandler(generalCommand *);

  QString recorderVersion;

  int32_t vfoFrequency;
  int16_t hwVersion;
  QSettings * sdrplaySettings;
  bool agcMode;
  const int16_t nrBits = 14;
  const int denominator = 8192;
  int lna_upperBound;
  float apiVersion;
  QString serial;
  bool has_antennaSelect;
  QString deviceModel;
  int GRdBValue;
  int lnaState;
  int ppmValue;
  HINSTANCE Handle;
  bool biasT;
  FILE * xmlDumper;
  XmlFileWriter * xmlWriter;
  bool setup_xmlDump();
  void close_xmlDump();
  std::atomic<bool> dumping;
  std::queue<generalCommand *> server_queue;
  QSemaphore serverjobs;
  HINSTANCE fetchLibrary();
  void releaseLibrary();
  bool loadFunctions();
  int errorCode;

private slots:
  void set_ifgainReduction(int);
  void set_lnagainReduction(int);
  void set_agcControl(int);
  void set_ppmControl(int);
  void set_selectAntenna(const QString &);
  void set_biasT(int);
  void slot_overload_detected(bool);
public slots:
  void set_lnabounds(int, int);
  //void set_nrBits(int);
  void set_deviceName(const QString &);
  void set_serial(const QString &);
  void set_apiVersion(float);
  void set_antennaSelect(int);
  void set_xmlDump();
  void show_lnaGain(int);
signals:
  void new_GRdBValue(int);
  void new_lnaValue(int);
  void new_agcSetting(bool);
  void set_lnabounds_signal(int, int);
  void set_deviceName_signal(const QString &);
  void set_serial_signal(const QString &);
  void set_apiVersion_signal(float);
  void set_antennaSelect_signal(bool);
  void signal_overload_detected(bool);
};

#endif

