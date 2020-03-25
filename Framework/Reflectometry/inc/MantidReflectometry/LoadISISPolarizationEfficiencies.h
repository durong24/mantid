// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidReflectometry/CreatePolarizationEfficienciesBase.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

/** LoadISISPolarizationEfficiencies : Load reflectometry polarization
  efficiency correction factors from disk.
*/
class MANTID_REFLECTOMETRY_DLL LoadISISPolarizationEfficiencies
    : public CreatePolarizationEfficienciesBase {
public:
  const std::string name() const override;
  int version() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &props) override;
};

} // namespace Reflectometry
} // namespace Mantid
