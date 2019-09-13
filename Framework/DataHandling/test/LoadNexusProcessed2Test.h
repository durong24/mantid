// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADNEXUSPROCESSED2TEST_H_
#define MANTID_DATAHANDLING_LOADNEXUSPROCESSED2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/LoadNexusProcessed2.h"
#include "MantidDataHandling/SaveNexusESS.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTestHelpers/FileResource.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid::DataHandling;
using namespace Mantid::API;

namespace {

template <typename Alg>
Mantid::API::MatrixWorkspace_sptr do_load(const std::string &filename) {
  Alg loader;
  loader.setChild(true);
  loader.setRethrows(true);
  loader.initialize();
  loader.setProperty("Filename", filename);
  loader.setPropertyValue("OutputWorkspace", "dummy");
  loader.execute();
  Workspace_sptr out = loader.getProperty("OutputWorkspace");
  auto matrixWSOut =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(out);
  return matrixWSOut;
}

Mantid::API::MatrixWorkspace_sptr do_load_v2(const std::string &filename) {
  return do_load<LoadNexusProcessed2>(filename);
}

Mantid::API::MatrixWorkspace_sptr do_load_v1(const std::string &filename) {
  return do_load<LoadNexusProcessed>(filename);
}

namespace test_utility {
template <typename T> void save(const std::string filename, T &ws) {
  SaveNexusESS alg;
  alg.setChild(true);
  alg.setRethrows(true);
  alg.initialize();
  alg.isInitialized();
  alg.setProperty("InputWorkspace", ws);
  alg.setProperty("Filename", filename);
  alg.execute();
  alg.isExecuted();
}

Mantid::API::MatrixWorkspace_sptr make_workspace(const std::string &filename) {
  LoadEmptyInstrument loader;
  loader.setChild(true);
  loader.initialize();
  loader.setProperty("Filename", filename);
  loader.setPropertyValue("OutputWorkspace", "dummy");
  loader.execute();
  MatrixWorkspace_sptr ws = loader.getProperty("OutputWorkspace");
  return ws;
}
} // namespace test_utility
} // namespace

class LoadNexusProcessed2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadNexusProcessed2Test *createSuite() {
    return new LoadNexusProcessed2Test();
  }
  static void destroySuite(LoadNexusProcessed2Test *suite) { delete suite; }

  void test_checkVersion() {
    LoadNexusProcessed2 alg;
    TS_ASSERT_EQUALS(alg.version(), 2);
  }

  void test_defaultVersion() {
    auto alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadNexusProcessed");
    TS_ASSERT_EQUALS(alg->version(), 2);
  }
  void test_with_ess_instrument() {

    using namespace Mantid::HistogramData;

    FileResource fileInfo("test_ess_instrument.nxs");

    auto wsIn =
        test_utility::make_workspace("V20_4-tubes_90deg_Definition_v01.xml");
    for (size_t i = 0; i < wsIn->getNumberHistograms(); ++i) {
      wsIn->setCounts(i, Counts{double(i)});
    }

    test_utility::save(fileInfo.fullPath(), wsIn);
    auto wsOut = do_load_v2(fileInfo.fullPath());

    // Quick geometry Test
    TS_ASSERT(wsOut->detectorInfo().isEquivalent(wsIn->detectorInfo()));

    // Quick data test.
    for (size_t i = 0; i < wsIn->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(wsIn->counts(i)[0], wsOut->counts(i)[0]);
    }
  }

  void test_reading_mappings() {
    using Mantid::SpectrumDefinition;
    using namespace Mantid::Indexing;
    FileResource fileInfo("test_no_spectra_mapping.nxs");
    auto wsIn =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            2 /*numBanks*/, 10 /*numPixels*/, 12 /*numBins*/);

    std::vector<SpectrumDefinition> specDefinitions;
    std::vector<SpectrumNumber> spectrumNumbers;
    size_t i = wsIn->getNumberHistograms() - 1;
    for (size_t j = 0; j < wsIn->getNumberHistograms(); --i, ++j) {
      specDefinitions.push_back(SpectrumDefinition(i));
      spectrumNumbers.push_back(SpectrumNumber(static_cast<int>(j)));
    }
    IndexInfo info(spectrumNumbers);
    info.setSpectrumDefinitions(specDefinitions);
    wsIn->setIndexInfo(info);
    test_utility::save(fileInfo.fullPath(), wsIn);

    // Reload it.
    auto matrixWSOut = do_load_v2(fileInfo.fullPath());

    const auto &inSpecInfo = wsIn->spectrumInfo();
    const auto &outSpecInfo = matrixWSOut->spectrumInfo();

    // Note we do not guarantee the preseveration of spectrum indexes during
    // deserialisation, so we need the maps to ensure we compare like for like.
    auto specToIndexOut = matrixWSOut->getSpectrumToWorkspaceIndexMap();
    auto specToIndexIn = wsIn->getSpectrumToWorkspaceIndexMap();

    auto indexInfo = matrixWSOut->indexInfo();

    TS_ASSERT_EQUALS(outSpecInfo.size(), inSpecInfo.size());
    for (size_t i = 0; i < outSpecInfo.size(); ++i) {

      auto specNumber = int(indexInfo.spectrumNumber(i));

      auto indexInInput = specToIndexIn.at(specNumber);
      auto indexInOutput = specToIndexOut.at(specNumber);

      // Output has no mapping, so for each spectrum have 0 detector indices
      TS_ASSERT_EQUALS(outSpecInfo.spectrumDefinition(indexInOutput).size(),
                       inSpecInfo.spectrumDefinition(indexInInput).size());
      // Compare actual detector indices for each spectrum when fixed as below
      TS_ASSERT_EQUALS(outSpecInfo.spectrumDefinition(indexInOutput)[0],
                       inSpecInfo.spectrumDefinition(indexInInput)[0]);
    }
  }

  void test_demonstrate_old_loader_incompatible() {

    FileResource fileInfo("test_demo_file_for_incompatible.nxs");

    auto wsIn =
        test_utility::make_workspace("V20_4-tubes_90deg_Definition_v01.xml");

    test_utility::save(fileInfo.fullPath(), wsIn);
    auto wsOut = do_load_v1(fileInfo.fullPath());
    // Should fail to handle ESS layout. Algorithm runs, but output not same as
    // input. i.e. No geometry
    TS_ASSERT(!wsOut->detectorInfo().isEquivalent(wsIn->detectorInfo()));
  }
};

#endif /* MANTID_DATAHANDLING_LOADNEXUSPROCESSED2TEST_H_ */