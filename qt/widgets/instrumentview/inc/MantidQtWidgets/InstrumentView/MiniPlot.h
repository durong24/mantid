// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/InstrumentView/MiniPlotQwt.h"
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/InstrumentView/MiniPlotMpl.h"
#endif

namespace MantidQt {
namespace MantidWidgets {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
using MiniPlot = MiniPlotQwt;
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
using MiniPlot = MiniPlotMpl;
#endif
} // namespace MantidWidgets
} // namespace MantidQt
