/*
 * Copyright (c) 2023 by Thomas Neder (https://github.com/tomneda)
 *
 * This project was originally forked from the project Qt-DAB by Jan van Katwijk. See https://github.com/JvanKatwijk/qt-dab.
 * Due to massive changes it got the new name DABstar. See: https://github.com/tomneda/DABstar
 *
 * DABstar is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the License, or any later version.
 *
 * DABstar is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with DABstar. If not, write to the Free Software
 * Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PHASE_CARR_DISP_H
#define PHASE_CARR_DISP_H

#include "glob_enums.h"
#include <qcolor.h>
#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <vector>
#include "qwt_defs.h"

class CarrierDisp : public QObject
{
Q_OBJECT
public:
  explicit CarrierDisp(QwtPlot * plot);
  ~CarrierDisp() = default;

  struct SCustPlot
  {
    enum class EStyle { DOTS, LINES, STICKS };

    ECarrierPlotType PlotType;
    EStyle Style;
    const char * Name;
    QString ToolTip;

    double YTopValue;
    double YBottomValue;
    int32_t YValueElementNo;

    int32_t MarkerYValueStep = 1; // if not each Y value a marker should set (0 = no set no marker)
  };

  void display_carrier_plot(const std::vector<TQwtData> & iYValVec);
  void select_plot_type(const ECarrierPlotType iPlotType);
  static QStringList get_plot_type_names();

private:
  QwtPlot * const mQwtPlot = nullptr;
  QwtPlotCurve mQwtPlotCurve;
  std::vector<QwtPlotMarker *> mQwtPlotMarkerVec;
  std::vector<TQwtData> mX_axis_vec;
  int32_t mDataSize = 0;
  ECarrierPlotType pt;

  void customize_plot(const SCustPlot & iCustPlot);
  static SCustPlot _get_plot_type_data(const ECarrierPlotType iPlotType);
  void _setup_x_axis();
private slots:
  void rightMouseClick(const QPointF &);
};

#endif // PHASE_CARR_DISP_H
