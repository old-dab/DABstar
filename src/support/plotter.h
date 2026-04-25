//
// Created by tomneda on 2026-04-25.
//
#pragma once

#include <array>
#include <fstream>
#include <string>
#include <cmath>

template <typename T, std::size_t N>
void plot_abs_array(const std::array<T, N>& iValues, const std::string& iTitle = "(no title)")
{
  const std::string dataFile = "/tmp/data_abs.dat";
  const std::string scriptFile = "/tmp/data_plot.gnuplot";

  {
    std::ofstream out(dataFile);
    for (std::size_t i = 0; i < iValues.size(); ++i)
    {
      out << i << " " << std::abs(iValues[i]) << '\n';
    }
  }

  {
    std::ofstream gp(scriptFile);
    gp << "set title '" << iTitle << "'\n";
    gp << "set xlabel 'Index'\n";
    gp << "set ylabel 'Magnitude'\n";
    gp << "set grid\n";
    gp << "plot '" << dataFile << "' using 1:2 with lines notitle\n";
    gp << "pause -1\n";
  }

  std::system(("gnuplot " + scriptFile).c_str());
}


