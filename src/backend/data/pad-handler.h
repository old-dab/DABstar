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

#ifndef  PAD_HANDLER_H
#define  PAD_HANDLER_H

#include  <QObject>
#include  <cstring>
#include  <cstdint>
#include  <vector>

class DabRadio;
class MotObject;

class PadHandler : public QObject
{
Q_OBJECT
public:
  explicit PadHandler(DabRadio *);
  ~PadHandler() override;

  void processPAD(uint8_t *, int16_t, uint8_t, uint8_t);

private:
  DabRadio * myRadioInterface;
  void handle_variablePAD(const uint8_t *, int16_t, uint8_t);
  void handle_shortPAD(const uint8_t *, int16_t, uint8_t);
  void dynamicLabel(const uint8_t *, int16_t, uint8_t);
  void new_MSC_element(const std::vector<uint8_t> &);
  void add_MSC_element(const std::vector<uint8_t> &);
  void build_MSC_segment(const std::vector<uint8_t> &);
  bool pad_crc(const uint8_t *, int16_t);
  void reset_charset_change();
  void check_charset_change();

  QByteArray dynamicLabelTextUnConverted;
  int16_t charSet;
  int16_t lastConvCharSet = -1;
  MotObject * currentSlide;
  uint8_t last_appType;
  bool mscGroupElement;
  int xpadLength;
  int16_t still_to_go;
  std::vector<uint8_t> shortpadData;
  bool lastSegment;
  bool firstSegment;
  int16_t segmentNumber;
  //      dataGroupLength is set when having processed an appType 1
  int dataGroupLength;

  //      The msc_dataGroupBuffer is - as the name suggests - used for
  //      assembling the msc_data group.
  std::vector<uint8_t> msc_dataGroupBuffer;

signals:
  void signal_show_label(const QString &);
  void signal_show_mot_handling(bool);
};

#endif
