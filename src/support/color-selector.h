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

#ifndef  COLOR_SELECTOR_H
#define  COLOR_SELECTOR_H

#include  <QDialog>
#include  <QLabel>
#include  <QListView>
#include  <QStringListModel>
#include  <QStringList>
#include  <cstdint>

class colorSelector : public QDialog
{
Q_OBJECT
public:
  colorSelector(const QString &);
  ~colorSelector() = default;

  QString getColor(int);

private:
  QLabel * mpToptext;
  QListView * mpSelectorDisplay;
  QStringListModel mColorList;
  QStringList mColors;
  int16_t mSelectedItem = 0;

private slots:
  void select_color(QModelIndex);
};

#endif

