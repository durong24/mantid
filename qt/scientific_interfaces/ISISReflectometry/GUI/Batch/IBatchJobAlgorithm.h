// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IBatchJobAlgorithm.h"
#include "MantidAPI/Workspace_fwd.h"

#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class Item;

class MANTIDQT_ISISREFLECTOMETRY_DLL IBatchJobAlgorithm {
public:
  virtual ~IBatchJobAlgorithm() = default;

  virtual Item *item() = 0;
  virtual void updateItem() = 0;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
