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
 *
 * 	File reader:
 *	For the (former) files with 8 bit raw data from the
 *	dabsticks
 */
#include  "rawfiles.h"
#include  "raw-reader.h"
#include  "openfiledialog.h"
#include  <cstdio>
#include  <cstdlib>
#include  <fcntl.h>
#include  <ctime>

#ifdef _WIN32
#else
  #include  <unistd.h>
  #include  <sys/time.h>
#endif

#define  INPUT_FRAMEBUFFERSIZE  8 * 32768

RawFileHandler::RawFileHandler(const QString & iFilename)
  : myFrame(nullptr)
  , _I_Buffer(INPUT_FRAMEBUFFERSIZE)
{
  fileName = iFilename;
  setupUi(&myFrame);
  myFrame.setWindowFlag(Qt::Tool, true); // does not generate a task bar icon
  myFrame.show();
  filePointer = OpenFileDialog::open_file(iFilename, "rb");
  if (filePointer == nullptr)
  {
    const QString val = QString("Cannot open file '%1'").arg(iFilename);
    throw std::runtime_error(val.toUtf8().data());
  }
  nameofFile->setText(iFilename);
  fseek(filePointer, 0, SEEK_END);
  int64_t fileLength = ftell(filePointer);
  totalTime->display(QString("%1").arg((float)fileLength / (2048000 * 2), 0, 'f', 1));
  fseek(filePointer, 0, SEEK_SET);
  fileProgress->setValue(0);
  currentTime->display(0);

  connect(cbLoopFile, &QCheckBox::clicked, this, &RawFileHandler::slot_handle_cb_loop_file);
  running.store(false);
}

RawFileHandler::~RawFileHandler()
{
  if (running.load())
  {
    readerTask->stopReader();
    while (readerTask->isRunning())
    {
      usleep(100);
    }
    delete readerTask;
  }
  if (filePointer != nullptr)
  {
    fclose(filePointer);
  }
}

bool RawFileHandler::restartReader(int32_t freq)
{
  (void)freq;
  if (running.load())
  {
    return true;
  }
  readerTask = new RawReader(this, filePointer, &_I_Buffer);
  running.store(true);
  return true;
}

void RawFileHandler::stopReader()
{
  if (running.load())
  {
    readerTask->stopReader();
    while (readerTask->isRunning())
    {
      usleep(100);
    }
    delete readerTask;
  }
  running.store(false);
}

//	size is in I/Q pairs, file contains 8 bits values
int32_t RawFileHandler::getSamples(cmplx * V, int32_t size)
{
  int32_t amount;

  if (filePointer == nullptr)
  {
    return 0;
  }

  while ((int32_t)(_I_Buffer.get_ring_buffer_read_available()) < size)
  {
    usleep(500);
  }

  amount = _I_Buffer.get_data_from_ring_buffer(V, size);
  return amount;
}

int32_t RawFileHandler::Samples()
{
  return _I_Buffer.get_ring_buffer_read_available();
}

void RawFileHandler::setProgress(int progress, float timelength)
{
  fileProgress->setValue(progress);
  currentTime->display(QString("%1").arg(timelength, 0, 'f', 1));
}

void RawFileHandler::show()
{
  myFrame.show();
}

void RawFileHandler::hide()
{
  myFrame.hide();
}

bool RawFileHandler::isHidden()
{
  return myFrame.isHidden();
}

bool RawFileHandler::isFileInput()
{
  return true;
}

void RawFileHandler::setVFOFrequency(int32_t)
{
}

int RawFileHandler::getVFOFrequency()
{
  return 0;
}

void RawFileHandler::resetBuffer()
{
}

int16_t RawFileHandler::bitDepth()
{
  return 10; // TODO: taken from former default interface, is it correct?
}

QString RawFileHandler::deviceName()
{
  return "RawFile";
}

void RawFileHandler::slot_handle_cb_loop_file(const bool iChecked)
{
  if (filePointer == nullptr)
  {
    return;
  }

  cbLoopFile->setChecked(readerTask->handle_continuousButton());
}

