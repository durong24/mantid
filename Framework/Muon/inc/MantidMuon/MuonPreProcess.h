// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_MUONPREPROCESS_H_
#define MANTID_MUON_MUONPREPROCESS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Muon {

class DLLExport MuonPreProcess : public API::Algorithm {
public:
  MuonPreProcess() : API::Algorithm() {}
  ~MuonPreProcess() {}

  const std::string name() const override { return "MuonPreProcess"; }
  int version() const override { return (1); }
  const std::string category() const override { return "Muon\\DataHandling"; }
  const std::string summary() const override {
    return "Perform a series of common analysis pre-processing steps on Muon "
           "data. Sample logs are modified to record the input parameters.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess"};
  }

private:
  void init() override;
  void exec() override;

  /// Apply a series of corrections ; DTC, offset, rebin, crop
  WorkspaceGroup_sptr correctWorkspaces(WorkspaceGroup_sptr wsGroup);
  MatrixWorkspace_sptr correctWorkspace(MatrixWorkspace_sptr ws);

  MatrixWorkspace_sptr applyDTC(MatrixWorkspace_sptr ws,
                                TableWorkspace_sptr dt);

  MatrixWorkspace_sptr applyTimeOffset(MatrixWorkspace_sptr ws,
                                       const double &offset);

  MatrixWorkspace_sptr applyCropping(MatrixWorkspace_sptr ws,
                                     const double &xMin, const double &xMax);

  MatrixWorkspace_sptr applyRebinning(MatrixWorkspace_sptr ws,
                                      const std::vector<double> &rebinArgs);

  /// Add the correction inputs into the logs
  void addPreProcessSampleLogs(WorkspaceGroup_sptr group);

  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;

  /// Allow WorkspaceGroup property to function correctly.
  bool checkGroups() override;
};

} // namespace Muon
} // namespace Mantid

#endif /* MANTID_MUON_MUONPREPROCESS_H_ */