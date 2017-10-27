#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SumSpectra)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

SumSpectra::SumSpectra()
    : API::Algorithm(), m_outSpecNum(0), m_keepMonitors(false),
      m_replaceSpecialValues(false), m_numberOfSpectra(0), m_yLength(0),
      m_indices(), m_calculateWeightedSum(false) {}

/** Initialisation method.
 *
 */
void SumSpectra::init() {
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "The workspace containing the spectra to be summed.");
  declareProperty(
      make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                       Direction::Output),
      "The name of the workspace to be created as the output of the algorithm. "
      " A workspace of this name will be created and stored in the Analysis "
      "Data Service.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "The first Workspace index to be included in the summing");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "The last Workspace index to be included in the summing");

  declareProperty(
      make_unique<Kernel::ArrayProperty<int>>("ListOfWorkspaceIndices"),
      "A list of workspace indices as a string with ranges, for "
      "example: 5-10,15,20-23. \n"
      "Optional: if not specified, then the "
      "Start/EndWorkspaceIndex fields are used alone. "
      "If specified, the range and the list are combined (without "
      "duplicating indices). For example, a range of 10 to 20 and "
      "a list '12,15,26,28' gives '10-20,26,28'.");

  declareProperty("IncludeMonitors", true,
                  "Whether to include monitor spectra in the summation.");

  declareProperty("WeightedSum", false,
                  "Instead of the usual spectra sum, calculate the weighted "
                  "sum. This has the form: \n"
                  ":math:`nSpectra "
                  "\\times\\Sigma(Signal_i/Error_i^2)/\\Sigma(1/Error_i^2)`\n "
                  "This property is ignored for event workspace.\n"
                  "The sums are defined for :math:`Error_i != 0` only, so the "
                  "values with zero error are dropped from the summation. To "
                  "estimate the number of dropped values see the "
                  "description. ");

  declareProperty("RemoveSpecialValues", false,
                  "If enabled floating point special values such as NaN or Inf"
                  " are removed before the spectra are summed.");
}

/*
* Validate the input parameters
* @returns map with keys corresponding to properties with errors and values
* containing the error messages.
*/
std::map<std::string, std::string> SumSpectra::validateInputs() {
  // create the map
  std::map<std::string, std::string> validationOutput;

  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");
  const int numSpectra =
      static_cast<int>(localworkspace->getNumberHistograms());
  const int minIndex = getProperty("StartWorkspaceIndex");
  const int maxIndex = getProperty("EndWorkspaceIndex");

  // check StartWorkSpaceIndex,  >=0 done by validator
  if (minIndex >= numSpectra) {
    validationOutput["StartWorkspaceIndex"] =
        "Selected minimum workspace index is greater than available spectra.";
  }

  // check EndWorkspaceIndex in range
  if (maxIndex != EMPTY_INT()) {
    // check EndWorkspaceIndex in range
    if (maxIndex >= numSpectra) {
      validationOutput["EndWorkspaceIndex"] =
          "Selected maximum workspace index is greater than available spectra.";
      // check StartWorkspaceIndex < EndWorkspaceIndex
    } else if (minIndex > maxIndex) {
      validationOutput["StartWorkspaceIndex"] =
          "Selected minimum workspace "
          "index is greater than selected "
          "maximum workspace index.";
      validationOutput["EndWorkspaceIndex"] =
          "Selected maximum workspace index "
          "is lower than selected minimum "
          "workspace index.";
    }
  }

  // check ListOfWorkspaceIndices in range
  const std::vector<int> indices_list = getProperty("ListOfWorkspaceIndices");
  if (!indices_list.empty()) { // only if specified
    // indices are assumed to be sorted
    for (const auto index : indices_list) {
      if ((index >= numSpectra) || (index < 0)) {
        validationOutput["ListOfWorkspaceIndices"] =
            "One or more indices out of range of available spectra.";
        break;
      }
    }
  }

  return validationOutput;
}

/** Executes the algorithm
 *
 */
void SumSpectra::exec() {
  // Try and retrieve the optional properties
  m_keepMonitors = getProperty("IncludeMonitors");
  m_replaceSpecialValues = getProperty("RemoveSpecialValues");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");
  m_numberOfSpectra = static_cast<int>(localworkspace->getNumberHistograms());
  determineIndices(m_numberOfSpectra);
  m_yLength = static_cast<int>(localworkspace->y(*(m_indices.begin())).size());

  // determine the output spectrum number
  m_outSpecNum = getOutputSpecNo(localworkspace);
  g_log.information()
      << "Spectra remapping gives single spectra with spectra number: "
      << m_outSpecNum << "\n";

  m_calculateWeightedSum = getProperty("WeightedSum");

  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(localworkspace);
  if (eventW) {
    if (m_calculateWeightedSum) {
      g_log.warning("Ignoring request for WeightedSum");
      m_calculateWeightedSum = false;
    }
    execEvent(eventW, m_indices);
  } else {
    //-------Workspace 2D mode -----

    // Create the 2D workspace for the output
    MatrixWorkspace_sptr outputWorkspace =
        API::WorkspaceFactory::Instance().create(
            localworkspace, 1, localworkspace->x(*(m_indices.begin())).size(),
            m_yLength);
    size_t numSpectra(0); // total number of processed spectra
    size_t numMasked(0);  // total number of the masked and skipped spectra
    size_t numZeros(0);   // number of spectra which have 0 value in the first
    // column (used in special cases of evaluating how good
    // Poissonian statistics is)

    Progress progress(this, 0.0, 1.0, m_indices.size());

    // This is the (only) output spectrum
    auto &outSpec = outputWorkspace->getSpectrum(0);

    // Copy over the bin boundaries
    outSpec.setSharedX(localworkspace->sharedX(0));

    // Build a new spectra map
    outSpec.setSpectrumNo(m_outSpecNum);
    outSpec.clearDetectorIDs();

    if (localworkspace->id() == "RebinnedOutput") {
      // this version is for a special workspace that has fractional overlap
      // information
      doFractionalSum(outputWorkspace, progress, numSpectra, numMasked,
                      numZeros);
    } else {
      // for things where all the bins are lined up
      doSimpleSum(outputWorkspace, progress, numSpectra, numMasked, numZeros);
    }

    auto &YError = outSpec.mutableE();
    // take the square root of all the accumulated squared errors - Assumes
    // Gaussian errors
    std::transform(YError.begin(), YError.end(), YError.begin(),
                   (double (*)(double))std::sqrt);

    // set up the summing statistics
    outputWorkspace->mutableRun().addProperty("NumAllSpectra", int(numSpectra),
                                              "", true);
    outputWorkspace->mutableRun().addProperty("NumMaskSpectra", int(numMasked),
                                              "", true);
    outputWorkspace->mutableRun().addProperty("NumZeroSpectra", int(numZeros),
                                              "", true);

    // Assign it to the output workspace property
    setProperty("OutputWorkspace", outputWorkspace);
  }
}

void SumSpectra::determineIndices(const int numberOfSpectra) {
  // assume that m_numberOfSpectra has been set
  m_indices.clear();

  // try the list form first
  const std::vector<int> indices_list = getProperty("ListOfWorkspaceIndices");
  m_indices.insert(indices_list.begin(), indices_list.end());

  // add the range specified by the user
  // this has been checked to be 0<= m_minWsInd <= maxIndex <=
  // m_numberOfSpectra where maxIndex can be an EMPTY_INT
  int minIndex = getProperty("StartWorkspaceIndex");
  int maxIndex = getProperty("EndWorkspaceIndex");
  if (isEmpty(maxIndex) && m_indices.empty()) {
    maxIndex = numberOfSpectra - 1;
  }

  // create the indices in the range
  if (!isEmpty(maxIndex)) {
    for (int i = minIndex; i <= maxIndex; i++)
      m_indices.insert(i);
  }
}

/**
 * Determine the minimum spectrum No for summing. This requires that
 * SumSpectra::indices has aly been set.
 * @param localworkspace The workspace to use.
 * @return The minimum spectrum No for all the spectra being summed.
 */
specnum_t
SumSpectra::getOutputSpecNo(MatrixWorkspace_const_sptr localworkspace) {
  // initial value - any included spectrum will do
  specnum_t specId =
      localworkspace->getSpectrum(*(m_indices.begin())).getSpectrumNo();

  // the total number of spectra
  int totalSpec = static_cast<int>(localworkspace->getNumberHistograms());

  specnum_t temp;
  for (const auto index : m_indices) {
    if (index < totalSpec) {
      temp = localworkspace->getSpectrum(index).getSpectrumNo();
      if (temp < specId)
        specId = temp;
    }
  }

  return specId;
}

/**
  * Calls an algorithm to replace special values within the workspace
  * such as NaN or Inf to 0.
  * @param inputWs The workspace to process
  * @return The workspace with special floating point values set to 0
  */
API::MatrixWorkspace_sptr SumSpectra::replaceSpecialValues() {
  // Get a copy of the input workspace
  MatrixWorkspace_sptr wksp = getProperty("InputWorkspace");

  if (!m_replaceSpecialValues) {
    // Skip any additional processing
    return wksp;
  }

  IAlgorithm_sptr alg = createChildAlgorithm("ReplaceSpecialValues");
  alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", wksp);
  std::string outName = "_" + wksp->getName() + "_clean";
  alg->setProperty("OutputWorkspace", outName);
  alg->setProperty("NaNValue", 0.0);
  alg->setProperty("NaNError", 0.0);
  alg->setProperty("InfinityValue", 0.0);
  alg->setProperty("InfinityError", 0.0);
  alg->executeAsChildAlg();
  return alg->getProperty("OutputWorkspace");
}

/**
 * This function deals with the logic necessary for summing a Workspace2D.
 * @param outSpec The spectrum for the summed output.
 * @param progress The progress indicator.
 * @param numSpectra The number of spectra contributed to the sum.
 * @param numMasked The spectra dropped from the summations because they are
 * masked.
 * @param numZeros The number of zero bins in histogram workspace or empty
 * spectra for event workspace.
 */
void SumSpectra::doSimpleSum(MatrixWorkspace_sptr outputWorkspace,
                             Progress &progress, size_t &numSpectra,
                             size_t &numMasked, size_t &numZeros) {
  // Clean workspace of any NANs or Inf values
  auto localworkspace = replaceSpecialValues();

  // Get references to the output workspaces's data vectors
  auto &outSpec = outputWorkspace->getSpectrum(0);
  auto &YSum = outSpec.mutableY();
  auto &YErrorSum = outSpec.mutableE();

  std::vector<double> Weight;
  std::vector<size_t> nZeros;
  if (m_calculateWeightedSum) {
    Weight.assign(YSum.size(), 0);
    nZeros.assign(YSum.size(), 0);
  }

  const auto &spectrumInfo = localworkspace->spectrumInfo();
  // Loop over spectra
  for (const auto wsIndex : m_indices) {
    if (spectrumInfo.hasDetectors(wsIndex)) {
      // Skip monitors, if the property is set to do so
      if (!m_keepMonitors && spectrumInfo.isMonitor(wsIndex))
        continue;
      // Skip masked detectors
      if (spectrumInfo.isMasked(wsIndex)) {
        numMasked++;
        continue;
      }
    }
    numSpectra++;

    const auto &YValues = localworkspace->y(wsIndex);
    const auto &YErrors = localworkspace->e(wsIndex);

    if (m_calculateWeightedSum) {
      // Retrieve the spectrum into a vector
      for (int yIndex = 0; yIndex < m_yLength; ++yIndex) {
        const double yErrorsVal = YErrors[yIndex];
        if (std::isnormal(yErrorsVal)) { // is non-zero, nan, or infinity
          const double errsq = yErrorsVal * yErrorsVal;
          YErrorSum[yIndex] += errsq;
          Weight[yIndex] += 1. / errsq;
          YSum[yIndex] += YValues[yIndex] / errsq;
        } else {
          nZeros[yIndex]++;
        }
      }
    } else {
      YSum += YValues;
      for (int yIndex = 0; yIndex < m_yLength; ++yIndex) {
        const auto yErrorsVal = YErrors[yIndex];
        YErrorSum[yIndex] += yErrorsVal * yErrorsVal;
      }
    }

    // Map all the detectors onto the spectrum of the output
    outSpec.addDetectorIDs(
        localworkspace->getSpectrum(wsIndex).getDetectorIDs());

    progress.report();
  }

  if (m_calculateWeightedSum) {
    for (size_t yIndex = 0; yIndex < static_cast<size_t>(m_yLength); yIndex++) {
      if (numSpectra > nZeros[yIndex])
        YSum[yIndex] *= double(numSpectra - nZeros[yIndex]) / Weight[yIndex];
      if (nZeros[yIndex] != 0)
        numZeros += nZeros[yIndex];
    }
  }
}

/**
 * This function handles the logic for summing RebinnedOutput workspaces.
 * @param outputWorkspace the workspace to hold the summed input
 * @param progress the progress indicator
 * @param numSpectra
 * @param numMasked
 * @param numZeros
 */
void SumSpectra::doFractionalSum(MatrixWorkspace_sptr outputWorkspace,
                                 Progress &progress, size_t &numSpectra,
                                 size_t &numMasked, size_t &numZeros) {
  // First, we need to clean the input workspace for nan's and inf's in order
  // to treat the data correctly later. This will create a new private
  // workspace that will be retrieved as mutable.
  auto localworkspace = replaceSpecialValues();

  // Transform to real workspace types
  RebinnedOutput_sptr inWS =
      boost::dynamic_pointer_cast<RebinnedOutput>(localworkspace);
  RebinnedOutput_sptr outWS =
      boost::dynamic_pointer_cast<RebinnedOutput>(outputWorkspace);

  // Get references to the output workspaces's data vectors
  auto &outSpec = outputWorkspace->getSpectrum(0);
  auto &YSum = outSpec.mutableY();
  auto &YErrorSum = outSpec.mutableE();
  auto &FracSum = outWS->dataF(0);

  std::vector<double> Weight;
  std::vector<size_t> nZeros;
  if (m_calculateWeightedSum) {
    Weight.assign(YSum.size(), 0);
    nZeros.assign(YSum.size(), 0);
  }

  const auto &spectrumInfo = localworkspace->spectrumInfo();
  // Loop over spectra
  for (const auto wsIndex : m_indices) {
    if (spectrumInfo.hasDetectors(wsIndex)) {
      // Skip monitors, if the property is set to do so
      if (!m_keepMonitors && spectrumInfo.isMonitor(wsIndex))
        continue;
      // Skip masked detectors
      if (spectrumInfo.isMasked(wsIndex)) {
        numMasked++;
        continue;
      }
    }
    numSpectra++;

    // Retrieve the spectrum into a vector
    const auto &YValues = localworkspace->y(wsIndex);
    const auto &YErrors = localworkspace->e(wsIndex);
    const auto &FracArea = inWS->readF(wsIndex);

    if (m_calculateWeightedSum) {
      for (int yIndex = 0; yIndex < m_yLength; ++yIndex) {
        if (YErrors[yIndex] != 0) {
          double errsq = YErrors[yIndex] * YErrors[yIndex] * FracArea[yIndex] *
                         FracArea[yIndex];
          YErrorSum[yIndex] += errsq;
          Weight[yIndex] += 1. / errsq;
          YSum[yIndex] += YValues[yIndex] * FracArea[yIndex] / errsq;
        } else {
          nZeros[yIndex]++;
        }
        FracSum[yIndex] += FracArea[yIndex];
      }
    } else {
      for (int yIndex = 0; yIndex < m_yLength; ++yIndex) {
        YSum[yIndex] += YValues[yIndex] * FracArea[yIndex];
        YErrorSum[yIndex] += YErrors[yIndex] * YErrors[yIndex] *
                             FracArea[yIndex] * FracArea[yIndex];
        FracSum[yIndex] += FracArea[yIndex];
      }
    }

    // Map all the detectors onto the spectrum of the output
    outSpec.addDetectorIDs(
        localworkspace->getSpectrum(wsIndex).getDetectorIDs());

    progress.report();
  }

  if (m_calculateWeightedSum) {
    for (size_t yIndex = 0; yIndex < static_cast<size_t>(m_yLength); yIndex++) {
      if (numSpectra > nZeros[yIndex])
        YSum[yIndex] *= double(numSpectra - nZeros[yIndex]) / Weight[yIndex];
      if (nZeros[yIndex] != 0)
        numZeros += nZeros[yIndex];
    }
  }

  // Create the correct representation
  outWS->finalize();
}

/** Executes the algorithm
 *@param localworkspace :: the input workspace
 *@param indices :: set of indices to sum up
 */
void SumSpectra::execEvent(EventWorkspace_const_sptr localworkspace,
                           std::set<int> &indices) {
  auto outputWorkspace =
      create<EventWorkspace>(*localworkspace, 1, localworkspace->binEdges(0));

  Progress progress(this, 0.0, 1.0, indices.size());

  // Get the pointer to the output event list
  EventList &outEL = outputWorkspace->getSpectrum(0);
  outEL.setSpectrumNo(m_outSpecNum);
  outEL.clearDetectorIDs();

  const auto &spectrumInfo = localworkspace->spectrumInfo();
  // Loop over spectra
  size_t numSpectra(0);
  size_t numMasked(0);
  size_t numZeros(0);
  for (const auto i : indices) {
    // Don't go outside the range.
    if ((i >= m_numberOfSpectra) || (i < 0)) {
      g_log.error() << "Invalid index " << i
                    << " was specified. Sum was aborted.\n";
      break;
    }

    if (spectrumInfo.hasDetectors(i)) {
      // Skip monitors, if the property is set to do so
      if (!m_keepMonitors && spectrumInfo.isMonitor(i))
        continue;
      // Skip masked detectors
      if (spectrumInfo.isMasked(i)) {
        numMasked++;
        continue;
      }
    }
    numSpectra++;

    // Add the event lists with the operator
    const EventList &tOutEL = localworkspace->getSpectrum(i);
    if (tOutEL.empty()) {
      ++numZeros;
    }
    outEL += tOutEL;

    progress.report();
  }

  outputWorkspace->mutableRun().addProperty("NumAllSpectra", int(numSpectra),
                                            "", true);
  outputWorkspace->mutableRun().addProperty("NumMaskSpectra", int(numMasked),
                                            "", true);
  outputWorkspace->mutableRun().addProperty("NumZeroSpectra", int(numZeros), "",
                                            true);

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", std::move(outputWorkspace));
}

} // namespace Algorithms
} // namespace Mantid
