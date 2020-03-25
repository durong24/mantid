// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SpecularReflectionAlgorithm.h"

namespace Mantid {
namespace Algorithms {

/** SpecularReflectionCorrectTheta : Calculates a theta value based on the
  specular reflection condition. Version 2.
*/
class MANTID_ALGORITHMS_DLL SpecularReflectionCalculateTheta2
    : public SpecularReflectionAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the specular reflection two theta scattering angle "
           "(degrees) from the detector and sample locations .";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SpecularReflectionPositionCorrect"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
