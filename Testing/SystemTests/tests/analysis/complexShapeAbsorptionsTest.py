# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import systemtesting
import mantid.simpleapi as mantid
from mantid import config
import os.path


DIRS = config['datasearch.directories'].split(';')
data_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(DIRS[0]))), "Data", "UnitTest")
wsfile = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(DIRS[0]))), "Data", "SystemTest", "WISH", "input",
                      "18_1", "WISH00040503.raw")


class SampleShapeBase(systemtesting.MantidSystemTest):

    def setup(self):
        mantid.Load(Filename=wsfile, OutputWorkspace="ws", SpectrumList="100")
        mantid.ConvertUnits(InputWorkspace="ws", Target="Wavelength", OutputWorkspace="ws")

    def runTest(self):
        self.setup()
        mantid.LoadSampleShape(InputWorkspace="ws", OutputWorkspace="ws",
                               Filename=os.path.join(data_dir, "cylinder.stl"))
        mantid.SetSampleMaterial(InputWorkspace="ws", ChemicalFormula="V", SampleNumberDensity=0.1)
        mantid.MonteCarloAbsorption(InputWorkspace="ws", OutputWorkspace="ws", NumberOfWavelengthPoints=50)

    def validate(self):
        return "ws", "complexShapeAbsorb.nxs"


class RotatedSampleShape(SampleShapeBase):
    def setup(self):
        mantid.Load(Filename=wsfile, OutputWorkspace="ws", SpectrumList="100")
        mantid.ConvertUnits(InputWorkspace="ws", Target="Wavelength", OutputWorkspace="ws")
        mantid.SetGoniometer(Workspace="ws", Axis0="90,1,0,0,1")

    def validate(self):
        return "ws", "complexShapeAbsorbRotated.nxs"
