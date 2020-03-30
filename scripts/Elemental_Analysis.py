# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore
from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui
from Muon.GUI.Common.usage_report import report_interface_startup


Name = "Elemental_Analysis"

if 'Elemental_Analysis' in globals():
    Elemental_Analysis = globals()['Elemental_Analysis']
    if not Elemental_Analysis.isHidden():
        Elemental_Analysis.setWindowState(
            Elemental_Analysis.windowState(
            ) & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
        Elemental_Analysis.activateWindow()
    else:
        Elemental_Analysis = ElementalAnalysisGui()
        report_interface_startup(Name)
        Elemental_Analysis.resize(700, 700)
        Elemental_Analysis.show()
else:
    Elemental_Analysis = ElementalAnalysisGui()
    report_interface_startup(Name)
    Elemental_Analysis.resize(700, 700)
    Elemental_Analysis.show()
