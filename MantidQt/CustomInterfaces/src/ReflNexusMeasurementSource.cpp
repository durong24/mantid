
#include "MantidQtCustomInterfaces/ReflNexusMeasurementSource.h"
#include <Poco/File.h>
#include <Poco/Exception.h>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <iostream>
#include <string>
#include <sstream>
#include <boost/regex.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReflNexusMeasurementSource::ReflNexusMeasurementSource() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflNexusMeasurementSource::~ReflNexusMeasurementSource() {}

Measurement
ReflNexusMeasurementSource::obtain(const std::string &definedPath,
                                   const std::string &fuzzyName) const {
  std::string filenameArg = fuzzyName;
  if (!definedPath.empty()) {
    Poco::File file(definedPath);
    try {
      if (file.exists() && file.isFile()) {
        // Load the exact path
        filenameArg = definedPath;
      }
    } catch (Poco::PathNotFoundException &) {
      /* Deliberately swallow the exception.
         Poco::File::exists throws for network drives
      */
    }
  }
  try {

    auto hostWorkspace =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    IAlgorithm_sptr algLoadRun =
        AlgorithmManager::Instance().create("LoadNexusLogs");
    algLoadRun->setChild(true);
    algLoadRun->setRethrows(true);
    algLoadRun->initialize();
    algLoadRun->setProperty("Filename", filenameArg);
    algLoadRun->setProperty("Workspace", hostWorkspace);
    algLoadRun->execute();

    auto run = hostWorkspace->run();
    const std::string measurementId =
        run.getPropertyValueAsType<std::string>("measurement_id");
    const std::string measurementSubId =
        run.getPropertyValueAsType<std::string>("measurement_subid");
    const std::string measurementLabel =
        run.getPropertyValueAsType<std::string>("measurement_label");
    const std::string measurementType =
        run.getPropertyValueAsType<std::string>("measurement_type");
    std::string runNumber;
    try {
      runNumber = run.getPropertyValueAsType<std::string>("run_number");
    } catch (Exception::NotFoundError &) {
      boost::regex re("([0-9]*)$");
      boost::smatch match;
      boost::regex_search(fuzzyName, match, re);
      runNumber = match[0];
    }

    double theta = -1.0;
    try {
      Property *prop = run.getProperty("stheta");
      if (TimeSeriesProperty<double> *tsp =
              dynamic_cast<TimeSeriesProperty<double> *>(prop)) {
        theta = tsp->valuesAsVector().back();
      }
    } catch (Exception::NotFoundError &) {
    }

    return Measurement(measurementId, measurementSubId, measurementLabel,
                       measurementType, theta, runNumber);

  } catch (std::invalid_argument &ex) {
    std::stringstream buffer;
    buffer << "Meta-data load attemped a load using: " << filenameArg
           << std::endl;
    buffer << ex.what();
    const std::string message = buffer.str();
    return Measurement::InvalidMeasurement(message);
  }
}

ReflNexusMeasurementSource *ReflNexusMeasurementSource::clone() const {
  return new ReflNexusMeasurementSource(*this);
}

} // namespace CustomInterfaces
} // namespace MantidQt
