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
 *    This file is part of the Qt-DAB (formerly SDR-J, JSDR).
 *    Many of the ideas as implemented in Qt-DAB are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are acknowledged.
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
#ifndef QT_AUDIO_H
#define QT_AUDIO_H

#include	<stdio.h>
#include	<QAudioOutput>
#include	<QStringList>
#include	"dab-constants.h"
#include	"audio-player.h"
#include	<QIODevice>
#include	<QScopedPointer>
#include	<QComboBox>
#include	<vector>
#include	<atomic>
#include	"ringbuffer.h"

class		QSettings;

class	Qt_Audio: public audioPlayer {
  Q_OBJECT
  public:
  Qt_Audio	(QSettings *);
  ~Qt_Audio	();
  void		stop		();
  void		restart		();
  void		suspend		();
  void		resume		();
  void		audioOutput	(float *, int32_t);
  QStringList	streams		();
  bool		selectDevice	(int16_t);
private:
  RingBuffer<char> tempBuffer;
  QSettings	*audioSettings;
  void		initialize_deviceList ();
  void		initializeAudio(const QAudioDeviceInfo &deviceInfo);
  QAudioFormat	audioFormat;
  QScopedPointer<QAudioOutput> m_audioOutput;
  int32_t		outputRate;
  std::vector<QAudioDeviceInfo> theList;
  std::atomic<bool>	isInitialized;
  std::atomic<bool>	working;
  QIODevice	*theWorker;
  int		newDeviceIndex;
  public slots:
    void		setVolume	(int);
};

#endif
