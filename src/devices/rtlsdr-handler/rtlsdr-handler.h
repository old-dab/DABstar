/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB
 *
 *    Many of the ideas as implemented in Qt-DAB are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
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
 */

#ifndef RTLSDR_HANDLER_H
#define  RTLSDR_HANDLER_H

#include  <QObject>
#include  <QSettings>
#include  <QString>
#include  <cstdio>
#include  <atomic>
#include  <QComboBox>
#include  "dab-constants.h"
#include  "fir-filters.h"
#include  "device-handler.h"
#include  "ringbuffer.h"
#include  "ui_rtlsdr-widget.h"
#include  <QLibrary>

class dll_driver;
class XmlFileWriter;

//	create typedefs for the library functions
typedef struct rtlsdr_dev rtlsdr_dev_t;

extern "C" {
typedef void (* rtlsdr_read_async_cb_t)(uint8_t * buf, uint32_t len, void * ctx);
typedef int (* pfnrtlsdr_open )(rtlsdr_dev_t **, uint32_t);
typedef int (* pfnrtlsdr_close)(rtlsdr_dev_t *);
typedef int (* pfnrtlsdr_get_usb_strings)(rtlsdr_dev_t *, char *, char *, char *);
typedef int (* pfnrtlsdr_set_center_freq)(rtlsdr_dev_t *, uint32_t);
typedef int (* pfnrtlsdr_set_tuner_bandwidth)(rtlsdr_dev_t *, uint32_t);
typedef uint32_t (* pfnrtlsdr_get_center_freq)(rtlsdr_dev_t *);
typedef int (* pfnrtlsdr_get_tuner_gains)(rtlsdr_dev_t *, int *);
typedef int (* pfnrtlsdr_set_tuner_gain_mode)(rtlsdr_dev_t *, int);
typedef int (* pfnrtlsdr_set_agc_mode)(rtlsdr_dev_t *, int);
typedef int (* pfnrtlsdr_set_sample_rate)(rtlsdr_dev_t *, uint32_t);
typedef int (* pfnrtlsdr_get_sample_rate)(rtlsdr_dev_t *);
typedef int (* pfnrtlsdr_set_tuner_gain)(rtlsdr_dev_t *, int);
typedef int (* pfnrtlsdr_get_tuner_gain)(rtlsdr_dev_t *);
typedef int (* pfnrtlsdr_reset_buffer)(rtlsdr_dev_t *);
typedef int (* pfnrtlsdr_read_async)(rtlsdr_dev_t *, rtlsdr_read_async_cb_t, void *, uint32_t, uint32_t);
typedef int (* pfnrtlsdr_set_bias_tee)(rtlsdr_dev_t *, int);
typedef int (* pfnrtlsdr_cancel_async)(rtlsdr_dev_t *);
typedef int (* pfnrtlsdr_set_direct_sampling)(rtlsdr_dev_t *, int);
typedef uint32_t (* pfnrtlsdr_get_device_count)();
typedef int (* pfnrtlsdr_set_freq_correction)(rtlsdr_dev_t *, int);
typedef int (* pfnrtlsdr_set_freq_correction_ppb)(rtlsdr_dev_t *, int);
typedef char * (* pfnrtlsdr_get_device_name)(int);
typedef int (* pfnrtlsdr_get_tuner_i2c_register)(rtlsdr_dev_t *dev, unsigned char* data, int *len, int *strength);
typedef int (* pfnrtlsdr_get_tuner_type)(rtlsdr_dev_t *dev);
}

//	This class is a simple wrapper around the
//	rtlsdr library that is read in  as dll (or .so file in linux)
//	It does not do any processing
class RtlSdrHandler final : public QObject, public IDeviceHandler, public Ui_dabstickWidget
{
Q_OBJECT
public:
  RtlSdrHandler(QSettings * s, const QString & recorderVersion);
  ~RtlSdrHandler() override;
  void setVFOFrequency(int32_t) override;
  int32_t getVFOFrequency() override;
  bool restartReader(int32_t) override;
  void stopReader() override;
  int32_t getSamples(cmplx *, int32_t) override;
  int32_t Samples() override;
  void resetBuffer() override;
  int16_t bitDepth() override;
  QString deviceName() override;
  void show() override;
  void hide() override;
  bool isHidden() override;
  bool isFileInput() override;
  int16_t maxGain();
  bool detect_overload(uint8_t *buf, int len);

  //	These need to be visible for the separate usb handling thread
  RingBuffer<std::complex<uint8_t>> _I_Buffer;
  pfnrtlsdr_read_async rtlsdr_read_async;
  struct rtlsdr_dev * theDevice;
  std::atomic<bool> isActive;

private:
  QFrame myFrame;
  QSettings * rtlsdrSettings;
  int32_t inputRate;
  int32_t deviceCount;
  QLibrary * phandle;
  dll_driver * workerHandle;
  int32_t lastFrequency;
  int16_t gainsCount;
  QString deviceModel;
  QString recorderVersion;
  FILE * xmlDumper;
  XmlFileWriter * xmlWriter;
  bool setup_xmlDump();
  void close_xmlDump();
  std::atomic<bool> xml_dumping;
  FILE * iqDumper;
  bool setup_iqDump();
  void close_iqDump();
  std::atomic<bool> iq_dumping;
  float mapTable[256];
  bool filtering;
  LowPassFIR theFilter;
  int currentDepth;
  int agcControl;
  void set_autogain(int);
  void enable_gainControl(int);
  int old_overload = 2;
  int old_gain = 0;

  //	here we need to load functions from the dll
  bool load_rtlFunctions();
  pfnrtlsdr_open rtlsdr_open;
  pfnrtlsdr_close rtlsdr_close;
  pfnrtlsdr_get_usb_strings rtlsdr_get_usb_strings;
  pfnrtlsdr_set_center_freq rtlsdr_set_center_freq;
  pfnrtlsdr_set_tuner_bandwidth rtlsdr_set_tuner_bandwidth;
  pfnrtlsdr_get_center_freq rtlsdr_get_center_freq;
  pfnrtlsdr_get_tuner_gains rtlsdr_get_tuner_gains;
  pfnrtlsdr_set_tuner_gain_mode rtlsdr_set_tuner_gain_mode;
  pfnrtlsdr_set_agc_mode rtlsdr_set_agc_mode;
  pfnrtlsdr_set_sample_rate rtlsdr_set_sample_rate;
  pfnrtlsdr_get_sample_rate rtlsdr_get_sample_rate;
  pfnrtlsdr_set_tuner_gain rtlsdr_set_tuner_gain;
  pfnrtlsdr_get_tuner_gain rtlsdr_get_tuner_gain;
  pfnrtlsdr_reset_buffer rtlsdr_reset_buffer;
  pfnrtlsdr_cancel_async rtlsdr_cancel_async;
  pfnrtlsdr_set_bias_tee rtlsdr_set_bias_tee;
  pfnrtlsdr_set_direct_sampling rtlsdr_set_direct_sampling;
  pfnrtlsdr_get_device_count rtlsdr_get_device_count;
  pfnrtlsdr_set_freq_correction rtlsdr_set_freq_correction;
  pfnrtlsdr_set_freq_correction_ppb rtlsdr_set_freq_correction_ppb;
  pfnrtlsdr_get_device_name rtlsdr_get_device_name;
  pfnrtlsdr_get_tuner_i2c_register rtlsdr_get_tuner_i2c_register;
  pfnrtlsdr_get_tuner_type rtlsdr_get_tuner_type;

private slots:
  void set_ExternalGain(int);
  void set_ppmCorrection(double);
  void set_bandwidth(int);
  void set_xmlDump();
  void set_iqDump();
  void set_filter(int);
  void set_biasControl(int);
  void handle_hw_agc();
  void handle_sw_agc();
  void handle_manual();
  void slot_timer(int);

signals:
  void signal_timer(int);
};

#endif

