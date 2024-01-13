#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB program
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
#include	<cstdio>
#include	<unistd.h>
#include	<cstdlib>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<ctime>
#include	<QString>
#include	"wavfiles.h"

#define	__BUFFERSIZE__	8 * 32768

	WavFileHandler::WavFileHandler (QString f):
	   myFrame (nullptr),
	   _I_Buffer (__BUFFERSIZE__) {
SF_INFO *sf_info;

	fileName	= f;
	setupUi (&myFrame);
  myFrame.setWindowFlag(Qt::Tool, true); // does not generate a task bar icon
	myFrame. show	();

	sf_info		= (SF_INFO *)alloca (sizeof (SF_INFO));
	sf_info	-> format	= 0;
	filePointer	= sf_open (f. toUtf8(). data(), SFM_READ, sf_info);
	if (filePointer == nullptr) {
	   fprintf (stderr, "file %s no legitimate sound file\n", 
	                                f. toUtf8().data());
	   throw (24);
	}
	if ((sf_info -> samplerate != 2048000) ||
	    (sf_info -> channels != 2)) {
	   fprintf (stderr, "This is not a recorded dab file, sorry\n");
	   sf_close (filePointer);
	   throw (25);
	}
	nameofFile	-> setText (f);
	fileProgress	-> setValue (0);
	currentTime	-> display (0);
	int64_t fileLength	= sf_seek (filePointer, 0, SEEK_END);
	totalTime	-> display ((float)fileLength / 2048000);
	running. store (false);
}
//
//	Note that running == true <==> readerTask has value assigned

	WavFileHandler::~WavFileHandler() {
	if (running. load()) {
	   readerTask	-> stopReader();
	   while (readerTask -> isRunning())
	      usleep (500);
	   delete readerTask;
	}
	if (filePointer != nullptr)
	   sf_close (filePointer);
}

bool	WavFileHandler::restartReader		(int32_t freq) {
	(void)freq;
	if (running. load())
           return true;
        readerTask      = new WavReader (this, filePointer, &_I_Buffer);
        running. store (true);
        return true;
}

void	WavFileHandler::stopReader() {
       if (running. load()) {
           readerTask   -> stopReader();
           while (readerTask -> isRunning())
              usleep (100);
	   delete readerTask;
        }
        running. store (false);
}

//	size is in I/Q pairs
int32_t	WavFileHandler::getSamples	(cmplx *V, int32_t size) {
int32_t	amount;
	
	if (filePointer == nullptr)
	   return 0;

	while (_I_Buffer. get_ring_buffer_read_available() < size)
	      usleep (100);

	amount = _I_Buffer. get_data_from_ring_buffer (V, size);
	
	return amount;
}

int32_t	WavFileHandler::Samples() {
	return _I_Buffer. get_ring_buffer_read_available();
}

void    WavFileHandler::setProgress (int progress, float timelength) {
        fileProgress      -> setValue (progress);
        currentTime       -> display (timelength);
}

void	WavFileHandler::show		() {
	myFrame. show ();
}

void	WavFileHandler::hide		() {
	myFrame. hide	();
}

bool	WavFileHandler::isHidden	() {
	return myFrame. isHidden ();
}

bool	WavFileHandler::isFileInput	() {
	return true;
}

void WavFileHandler::setVFOFrequency(int32_t)
{
}

int WavFileHandler::getVFOFrequency()
{
  return 0;
}

void WavFileHandler::resetBuffer()
{
}

int16_t WavFileHandler::bitDepth()
{
  return 10; // TODO: taken from former default interface, is it correct?
}

QString WavFileHandler::deviceName()
{
  return "WavFile";
}