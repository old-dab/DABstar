/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef  IP_DATAHANDLER_H
#define  IP_DATAHANDLER_H

#include  "dab-constants.h"
#include  "virtual-datahandler.h"
#include  <vector>
#include  "ringbuffer.h"

class DabRadio;

class ip_dataHandler : public virtual_dataHandler
{
Q_OBJECT
public:
  ip_dataHandler(DabRadio *, RingBuffer<u8> *);
  ~ip_dataHandler() override = default;

  void add_mscDatagroup(const std::vector<u8> &) override ;

private:
  void process_ipVector(const std::vector<u8> &);
  void process_udpVector(const u8 *, i16);

  i16 handledPackets;
  RingBuffer<u8> * dataBuffer;

signals:
  void writeDatagram(int);
};

#endif



