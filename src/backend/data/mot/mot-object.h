/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2015 .. 2017
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

#ifndef  MOT_OBJECT_H
#define  MOT_OBJECT_H

#include  "dab-constants.h"
#include  "mot-content-types.h"
#include  <QObject>
#include  <QImage>
#include  <QLabel>
#include  <QByteArray>
#include  <QString>
#include  <QDir>
#include  <map>
#include  <iterator>

class DabRadio;

class MotObject : public QObject
{
Q_OBJECT
public:
  MotObject(DabRadio * mr, bool dirElement, uint16_t transportId, const uint8_t * segment, int32_t segmentSize, bool lastFlag);
  ~MotObject() override = default;

  void addBodySegment(const uint8_t * bodySegment, int16_t segmentNumber, int32_t segmentSize, bool lastFlag);
  uint16_t get_transportId();
  int get_headerSize();
private:
  bool dirElement;
  QString picturePath;
  uint16_t transportId;
  int16_t numofSegments;
  int32_t segmentSize;
  uint32_t headerSize;
  uint32_t bodySize;
  MOTContentType contentType;
  QString name;
  void handleComplete();
  std::map<int, QByteArray> motMap;

signals:
  void the_picture(QByteArray, int, QString);
  void handle_motObject(QByteArray, QString, int, bool);
};

#endif

