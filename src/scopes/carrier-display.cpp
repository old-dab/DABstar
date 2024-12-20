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

#include "glob_defs.h"
#include "carrier-display.h"
#include <qwt_scale_div.h>
#include <qwt_plot_zoomer.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <QPen>

CarrierDisp::CarrierDisp(QwtPlot * ipPlot) :
  mQwtPlot(ipPlot)
{
  mQwtPlotCurve.setPen(QPen(Qt::yellow, 2.0));
  mQwtPlotCurve.setOrientation(Qt::Vertical);
  mQwtPlotCurve.attach(mQwtPlot);

  QwtPlotPicker *lm_picker = new QwtPlotPicker(ipPlot->canvas());
  QwtPickerMachine *lpickerMachine = new QwtPickerClickPointMachine();
  lm_picker->setStateMachine(lpickerMachine);
  lm_picker->setMousePattern(QwtPlotPicker::MouseSelect1, Qt::RightButton);
  connect(lm_picker, SIGNAL(selected(const QPointF&)), this, SLOT(rightMouseClick(const QPointF &)));
  // panning with the left mouse button
  (void)new QwtPlotPanner(ipPlot->canvas());

  // zoom in/out with the wheel
  (void)new QwtPlotMagnifier(ipPlot->canvas());
  select_plot_type(ECarrierPlotType::DEFAULT);
}

void CarrierDisp::display_carrier_plot(const std::vector<TQwtData> & iYValVec)
{
  if (mDataSize != (int32_t)iYValVec.size())
  {
    mDataSize = (int32_t)iYValVec.size();
    _setup_x_axis();
  }

  mQwtPlotCurve.setSamples(mX_axis_vec.data(), iYValVec.data(), mDataSize);
  mQwtPlot->replot();
}

void CarrierDisp::select_plot_type(const ECarrierPlotType iPlotType)
{
  pt = iPlotType;
  customize_plot(_get_plot_type_data(iPlotType));
}

void CarrierDisp::customize_plot(const SCustPlot & iCustPlot)
{
  mQwtPlot->setToolTip(iCustPlot.ToolTip);

  assert(iCustPlot.YValueElementNo >= 2);
  assert(iCustPlot.YTopValue > iCustPlot.YBottomValue);

  const double diffVal = (iCustPlot.YTopValue - iCustPlot.YBottomValue) / (iCustPlot.YValueElementNo - 1);

  // draw vertical ticks
  {
    QList<double> tickList;

    for (int32_t elemIdx = 0; elemIdx < iCustPlot.YValueElementNo; ++elemIdx)
    {
      tickList.push_back(iCustPlot.YBottomValue + elemIdx * diffVal);
    }

    mQwtPlot->setAxisScaleDiv(QwtPlot::yLeft, QwtScaleDiv(tickList[0], tickList[tickList.size() - 1], QList<double>(), QList<double>(), tickList));
  }

  // draw horizontal marker lines at -90, 0 , 90 degrees
  {
    // delete old markers
    for (auto & p : mQwtPlotMarkerVec)
    {
      p->detach();
      delete p;
      p = nullptr;
    }

    if (iCustPlot.MarkerYValueStep > 0)
    {
      const int32_t noMarkers = (iCustPlot.YValueElementNo + iCustPlot.MarkerYValueStep - 1) / iCustPlot.MarkerYValueStep;
      mQwtPlotMarkerVec.resize(noMarkers);

      for (int32_t markerIdx = 0; markerIdx < noMarkers; ++markerIdx)
      {
        mQwtPlotMarkerVec[markerIdx] = new QwtPlotMarker();
        QwtPlotMarker * const p = mQwtPlotMarkerVec[markerIdx];
        p->setLineStyle(QwtPlotMarker::HLine);
        if (iCustPlot.Style == SCustPlot::EStyle::LINES)
        {
          p->setLinePen(QColor(Qt::darkGray), 0.0, Qt::DashLine);
        }
        else
        {
          p->setLinePen(QColor(Qt::gray), 0.0, Qt::SolidLine);
        }
        p->setValue(0, iCustPlot.YBottomValue + diffVal * markerIdx * iCustPlot.MarkerYValueStep);
        p->attach(mQwtPlot);
      }
    }
  }
  if(iCustPlot.Style == SCustPlot::EStyle::DOTS) mQwtPlotCurve.setStyle(QwtPlotCurve::Dots);
  else if(iCustPlot.Style == SCustPlot::EStyle::LINES) mQwtPlotCurve.setStyle(QwtPlotCurve::Lines);
  else mQwtPlotCurve.setStyle(QwtPlotCurve::Sticks);
}

void CarrierDisp::_setup_x_axis()
{
  const int32_t displaySizeHalf = mDataSize / 2;
  mX_axis_vec.resize(mDataSize);

  // the vector iPhaseVec does not contain data for the zero point, so skip the zero also in the x-vector
  for (int32_t i = 0; i < displaySizeHalf; i++)
  {
    mX_axis_vec[i] = (TQwtData)(i - displaySizeHalf);
    mX_axis_vec[i + displaySizeHalf] = (TQwtData)(i + 1);
  }

  mQwtPlot->setAxisScale(QwtPlot::xBottom, mX_axis_vec[0], mX_axis_vec[mX_axis_vec.size() - 1]);
}

CarrierDisp::SCustPlot CarrierDisp::_get_plot_type_data(const ECarrierPlotType iPlotType)
{
  SCustPlot cp;
  cp.PlotType = iPlotType;

  switch (iPlotType)
  {
  case ECarrierPlotType::SB_WEIGHT:
    cp.ToolTip = "Shows the soft-bit weight in percentage for each OFDM carrier.<p>This is used for the viterbi decoder data input.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "Soft-Bit Weight";
    cp.YTopValue = 100.0;
    cp.YBottomValue = 0.0;
    cp.YValueElementNo = 6;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::EVM_DB:
    cp.ToolTip = "Shows the EVM (Error Vector Magnitude) in dB of each OFDM carrier.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "EVM (dB)";
    cp.YTopValue = 0.0;
    cp.YBottomValue = -18.0;
    cp.YValueElementNo = 7;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::EVM_PER:
    cp.ToolTip = "Shows the EVM (Error Vector Magnitude) in percentage of each OFDM carrier.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "EVM (%)";
    cp.YTopValue = 100.0;
    cp.YBottomValue = 0.0;
    cp.YValueElementNo = 6;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::STD_DEV:
    cp.ToolTip = "Shows the standard deviation of the absolute phase (mapped to first quadrant) in degrees of each OFDM carrier.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "Phase Std-Dev.";
    cp.YTopValue = 45.0;
    cp.YBottomValue = 0.0;
    cp.YValueElementNo = 7;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::PHASE_ERROR:
    cp.ToolTip = "Shows the corrected phase error in degree of each OFDM carrier.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "Corr. Phase Error";
    cp.YTopValue = 20.0;
    cp.YBottomValue = -20.0;
    cp.YValueElementNo = 5;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::FOUR_QUAD_PHASE:
    cp.ToolTip = "Shows the 4 phase segments in degree for each OFDM carrier.";
    cp.Style = SCustPlot::EStyle::DOTS;
    cp.Name = "4-quadr. Phase";
    cp.YTopValue = 180.0;
    cp.YBottomValue = -180.0;
    cp.YValueElementNo = 9;
    cp.MarkerYValueStep = 2;
    break;

  case ECarrierPlotType::REL_POWER:
    cp.ToolTip = "Shows the relative signal power to the overall medium signal power in dB of each OFDM carrier.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "Relative Power";
    cp.YTopValue = 12.0;
    cp.YBottomValue = -24.0;
    cp.YValueElementNo = 7;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::SNR:
    cp.ToolTip = "Shows the Signal-Noise-Ratio (SNR) in dB of each OFDM carrier.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "S/N-Ratio";
    cp.YTopValue = 60.0;
    cp.YBottomValue = 0.0;
    cp.YValueElementNo = 7;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::NULL_TII:
    cp.ToolTip = "Shows the averaged null symbol level with TII carriers in percentage of the maximum peak.";
    cp.Style = SCustPlot::EStyle::STICKS;
    cp.Name = "Null Sym. TII";
    cp.YTopValue = 100.0;
    cp.YBottomValue = 0.0;
    cp.YValueElementNo = 6;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::NULL_NO_TII:
    cp.ToolTip = "Shows the averaged null symbol level without TII carriers in percentage of the maximum peak.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "Null Sym. no TII";
    cp.YTopValue = 100.0;
    cp.YBottomValue = 0.0;
    cp.YValueElementNo = 6;
    cp.MarkerYValueStep = 1;
    break;

  case ECarrierPlotType::NULL_OVR_POW:
    cp.ToolTip = "Shows the averaged null symbol (without TII) power relation to the averaged overall carrier power in dB.<p>"
                 "This reveals disturbing (non-DAB) signals which can degrade decoding quality.";
    cp.Style = SCustPlot::EStyle::LINES;
    cp.Name = "Null Sym. ovr. Pow.";
    cp.YTopValue = 6.0;
    cp.YBottomValue = -42.0;
    cp.YValueElementNo = 9;
    cp.MarkerYValueStep = 1;
    break;
  }

  cp.ToolTip += "<p>The carrier at index 0 is not really existing. The value is interpolated between the two neighbor carriers.";

  return cp;
}

QStringList CarrierDisp::get_plot_type_names()
{
  QStringList sl;

  // ATTENTION: use same sequence as in ECarrierPlotType
  sl << _get_plot_type_data(ECarrierPlotType::SB_WEIGHT).Name;
  sl << _get_plot_type_data(ECarrierPlotType::EVM_DB).Name;
  sl << _get_plot_type_data(ECarrierPlotType::EVM_PER).Name;
  sl << _get_plot_type_data(ECarrierPlotType::STD_DEV).Name;
  sl << _get_plot_type_data(ECarrierPlotType::PHASE_ERROR).Name;
  sl << _get_plot_type_data(ECarrierPlotType::FOUR_QUAD_PHASE).Name;
  sl << _get_plot_type_data(ECarrierPlotType::REL_POWER).Name;
  sl << _get_plot_type_data(ECarrierPlotType::SNR).Name;
  sl << _get_plot_type_data(ECarrierPlotType::NULL_TII).Name;
  sl << _get_plot_type_data(ECarrierPlotType::NULL_NO_TII).Name;
  sl << _get_plot_type_data(ECarrierPlotType::NULL_OVR_POW).Name;

  return sl;
}

void CarrierDisp::rightMouseClick(const QPointF &point)
{
  (void)point;
  _setup_x_axis();
  select_plot_type(pt);
}
