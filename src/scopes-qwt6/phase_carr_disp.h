//
// Created by Thomas Neder
//
#ifndef PHASE_CARR_DISP_H
#define PHASE_CARR_DISP_H

#include <qcolor.h>
#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <vector>

class CarrierDisp
{
public:
  explicit CarrierDisp(QwtPlot * plot);
  ~CarrierDisp() = default;

  enum class EPlotType
  {
    PHASE,
    MODQUAL,
    NULLTII
  };

  struct SCustPlot
  {
    enum class EStyle { DOTS, LINES };

    EPlotType PlotType = EPlotType::PHASE;
    EStyle Style = EStyle::DOTS;
    const char * Name = "(dummy)";

    double StartValue = -180.0;
    double StopValue = 180.0;
    int32_t Segments = 8; // each 45.0

    double MarkerStartValue = -90.0;
    double MarkerStopValue = 90.0;
    int32_t MarkerSegments = 3; // each 90.0
  };

  void disp_carrier_plot(const std::vector<float> & iPhaseVec);
  void customize_plot(const SCustPlot & iCustPlot);
  void select_plot_type(const EPlotType iPlotType);
  static QStringList get_plot_type_names() ;

private:
  QwtPlot * const mQwtPlot = nullptr;
  QwtPlotCurve mQwtPlotCurve;
  std::vector<QwtPlotMarker *> mQwtPlotMarkerVec;
  int32_t mDataSize = 0;
  std::vector<float> mX_axis_vec;

  static SCustPlot _get_plot_type_data(const EPlotType iPlotType);
  void _setup_x_axis();
};

#endif // PHASE_CARR_DISP_H
