# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from Muon.GUI.FrequencyDomainAnalysis.frequency_domain_analysis_2 import FrequencyAnalysisGui
from qtpy import QtCore
from Muon.GUI.Common.usage_report import report_interface_startup


Name = "Frequency_Domain_Analysis_2"

if 'muon_freq' in globals():
    muon_freq = globals()['muon_freq']
    if not muon_freq.isHidden():
        muon_freq.setWindowState(
            muon_freq.windowState(
            ) & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
        muon_freq.activateWindow()
    else:
        muon_freq = FrequencyAnalysisGui()
        report_interface_startup(Name)
        muon_freq.resize(700, 700)
        muon_freq.show()
else:
    muon_freq = FrequencyAnalysisGui()
    report_interface_startup(Name)
    muon_freq.resize(700, 700)
    muon_freq.show()
