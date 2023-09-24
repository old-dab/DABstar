/*
 * This file is adapted by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the  Qt-DAB program
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

#include  "preset-handler.h"
#include  "radio.h"
#include  <QComboBox>

PresetHandler::PresetHandler(RadioInterface * radio)
{
  this->mpRadioIf = radio;
  this->mFileName = "";
}

void PresetHandler::loadPresets(const QString & fileName, QComboBox * cb)
{
  QDomDocument xmlBOM;
  QFile f(fileName);

  this->mFileName = fileName;
  if (!f.open(QIODevice::ReadOnly))
  {
    return;
  }

  xmlBOM.setContent(&f);
  f.close();
  QDomElement root = xmlBOM.documentElement();
  QDomElement component = root.firstChild().toElement();
  while (!component.isNull())
  {
    if (component.tagName() == "PRESET_ELEMENT")
    {
      presetData pd;
      pd.serviceName = component.attribute("SERVICE_NAME", "???");
      pd.channel = component.attribute("CHANNEL", "5A");
      cb->addItem(pd.channel + ":" + pd.serviceName);
    }
    component = component.nextSibling().toElement();
  }
}

void PresetHandler::savePresets(const QComboBox * cb)
{
  QDomDocument the_presets;
  QDomElement root = the_presets.createElement("preset_db");

  the_presets.appendChild(root);

  for (int i = 1; i < cb->count(); i++)
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 2)
    QStringList list = cb->itemText(i).split(":", Qt::SkipEmptyParts);
#else
    QStringList list = cb -> itemText (i).split (":", QString::SkipEmptyParts);
#endif
    if (list.length() != 2)
    {
      continue;
    }
    const QString & channel = list.at(0);
    const QString & serviceName = list.at(1);
    QDomElement presetService = the_presets.createElement("PRESET_ELEMENT");
    presetService.setAttribute("SERVICE_NAME", serviceName);
    presetService.setAttribute("CHANNEL", channel);
    root.appendChild(presetService);
  }

  QFile file(this->mFileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    return;
  }

  QTextStream stream(&file);
  stream << the_presets.toString();
  file.close();
}

