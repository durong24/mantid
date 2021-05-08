// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** ExtractUnmaskedSpectra : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL ExtractUnmaskedSpectra : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"RemoveMaskedSpectra"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
