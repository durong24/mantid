# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.kernel import UnitConversion, DeltaEModeType
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace, Multiply, mtd
import scipy.constants as constants
import h5py
import numpy


def sample_angle(run):
    """Return the sample theta angle in degrees."""
    if isinstance(run, list):
        run = run[0]
    with h5py.File(run, "r") as nexus:
        if nexus.get('entry0/instrument/SAN') is not None:
            return float(numpy.array(nexus.get('entry0/instrument/SAN/value'), dtype='float'))
        elif nexus.get('entry0/instrument/san') is not None:
            return float(numpy.array(nexus.get('entry0/instrument/san/value'), dtype='float'))
        else:
            raise RuntimeError('Cannot retrieve sample angle from Nexus file {}.'.format(run))


def detector_angle(run):
    """Return the detector angle in degrees."""
    if isinstance(run, list):
        run = run[0]
    with h5py.File(run, "r") as nexus:
        if nexus.get('entry0/instrument/DAN') is not None:
            return float(numpy.array(nexus.get('entry0/instrument/DAN/value'), dtype='float'))
        elif nexus.get('entry0/instrument/dan') is not None:
            return float(numpy.array(nexus.get('entry0/instrument/dan/value'), dtype='float'))
        else:
            raise RuntimeError('Cannot retrieve detector angle from Nexus file {}.'.format(run))


def chopperOpeningAngle(sampleLogs, instrumentName):
    """Return the chopper opening angle in degrees."""
    if instrumentName == 'D17':
        chopper1Phase = sampleLogs.getProperty('VirtualChopper.chopper1_phase_average').value
        chopperWindow = sampleLogs.getProperty('ChopperWindow').value
        if chopper1Phase > 360.:
            # Workaround for broken old D17 NeXus files.
            chopper1Phase = sampleLogs.getProperty('VirtualChopper.chopper2_speed_average').value
        chopper2Phase = sampleLogs.getProperty('VirtualChopper.chopper2_phase_average').value
        openoffset = sampleLogs.getProperty('VirtualChopper.open_offset').value
        return chopperWindow - (chopper2Phase - chopper1Phase) - openoffset
    else:
        firstChopper = int(sampleLogs.getProperty('ChopperSetting.firstChopper').value)
        secondChopper = int(sampleLogs.getProperty('ChopperSetting.secondChopper').value)
        phase1Entry = 'CH{}.phase'.format(firstChopper)
        phase2Entry = 'CH{}.phase'.format(secondChopper)
        chopper1Phase = sampleLogs.getProperty(phase1Entry).value
        chopper2Phase = sampleLogs.getProperty(phase2Entry).value
        if chopper1Phase > 360.:
            # CH1.phase on FIGARO is set to an arbitrary value (999.9)
            chopper1Phase = 0.
        if sampleLogs.hasProperty('CollAngle.open_offset'):
            openoffset = sampleLogs.getProperty('CollAngle.open_offset').value
        else:
            openoffset = sampleLogs.getProperty('CollAngle.openOffset').value
        return 45. - (chopper2Phase - chopper1Phase) - openoffset


def chopperPairDistance(sampleLogs, instrumentName):
    """Return the gap between the two choppers."""
    if instrumentName == 'D17':
        return sampleLogs.getProperty('Distance.ChopperGap').value # in [m]
    else:
        return sampleLogs.getProperty('ChopperSetting.distSeparationChopperPair').value * 1e-3


def chopperSpeed(sampleLogs, instrumentName):
    """Return the chopper speed."""
    if instrumentName == 'D17':
        return sampleLogs.getProperty('VirtualChopper.chopper1_speed_average').value
    else:
        firstChopper = int(sampleLogs.getProperty('ChopperSetting.firstChopper').value)
        speedEntry = 'CH{}.rotation_speed'.format(firstChopper)
        return sampleLogs.getProperty(speedEntry).value


def correctForChopperOpenings(ws, directWS, names, cleanup, logging):
    """Correct reflectivity values if chopper openings between RB and DB differ."""
    def opening(instrumentName, logs, Xs):
        chopperGap = chopperPairDistance(logs, instrumentName)
        chopperPeriod = 60. / chopperSpeed(logs, instrumentName)
        openingAngle = chopperOpeningAngle(logs, instrumentName)
        return chopperGap * constants.m_n / constants.h / chopperPeriod * Xs * 1e-10 + openingAngle / 360.
    instrumentName = ws.getInstrument().getName()
    Xs = ws.readX(0)
    if ws.isHistogramData():
        Xs = (Xs[:-1] + Xs[1:]) / 2.
    reflectedOpening = opening(instrumentName, ws.run(), Xs)
    directOpening = opening(instrumentName, directWS.run(), Xs)
    corFactorWSName = names.withSuffix('chopper_opening_correction_factors')
    corFactorWS = CreateWorkspace(
        OutputWorkspace=corFactorWSName,
        DataX=ws.readX(0),
        DataY= directOpening / reflectedOpening,
        UnitX=ws.getAxis(0).getUnit().unitID(),
        ParentWorkspace=ws,
        EnableLogging=logging)
    correctedWSName = names.withSuffix('corrected_by_chopper_opening')
    correctedWS = Multiply(
        LHSWorkspace=ws,
        RHSWorkspace=corFactorWS,
        OutputWorkspace=correctedWSName,
        EnableLogging=logging)
    cleanup.cleanup(corFactorWS)
    cleanup.cleanup(ws)
    return correctedWS


def detectorResolution():
    """Return the detector resolution in mm."""
    return 0.0022


def pixelSize(instrumentName):
    """Return the pixel size in mm."""
    return 0.001195 if instrumentName == 'D17' else 0.0012


def deflectionAngle(sampleLogs):
    """Return the deflection angle in degree."""
    if sampleLogs.hasProperty('CollAngle.actual_coll_angle'):
        # Must be FIGARO
        return sampleLogs.getProperty('CollAngle.actual_coll_angle').value
    else:
        return 0.0


def slitSizeLogEntry(instrumentName, slitNumber):
    """Return the sample log entry which contains the slit size for the given slit"""
    if slitNumber not in [1, 2]:
        raise RuntimeError('Slit number out of range.')
    entry = 'VirtualSlitAxis.s{}w_actual_width' if instrumentName == 'D17' else 'VirtualSlitAxis.S{}H_actual_height'
    return entry.format(slitNumber + 1)


def inTOF(value, l1, l2):
    """Return the number (tof) converted to wavelength"""
    return UnitConversion.run('Wavelength', 'TOF', value, l1, l2, 0., DeltaEModeType.Elastic, 0.)


def instrumentName(ws):
    """Return the instrument's name validating it is either D17 or FIGARO."""
    name = ws.getInstrument().getName()
    if name != 'D17' and name != 'FIGARO':
        raise RuntimeError('Unrecognized instrument {}. Only D17 and FIGARO are supported.'.format(name))
    return name


def slitSizes(ws):
    run = ws.run()
    instrName = instrumentName(ws)
    slit2width = run.get(slitSizeLogEntry(instrName, 1))
    slit3width = run.get(slitSizeLogEntry(instrName, 2))
    if slit2width is None or slit3width is None:
        run.addProperty(SampleLogs.SLIT2WIDTH, str('-'), '', True)
        run.addProperty(SampleLogs.SLIT3WIDTH, str('-'), '', True)
    else:
        slit2widthUnit = slit2width.units
        slit3widthUnit = slit3width.units
        slit3w = slit3width.value
        if instrumentName(ws) != 'D17':
            bgs3 = float(run.getProperty('BGS3.value').value)
            if bgs3 >= 150.:
                slit3w += 0.08
            elif 150. > bgs3 >= 50.:
                slit3w += 0.06
            elif -50. > bgs3 >= -150.:
                slit3w -= 0.12
            elif bgs3 < -150.:
                slit3w -= 0.24
        slit2w = slit2width.value
        run.addProperty(SampleLogs.SLIT2WIDTH, float(slit2w), slit2widthUnit, True)
        run.addProperty(SampleLogs.SLIT3WIDTH, float(slit3w), slit3widthUnit, True)


class SampleLogs:
    FOREGROUND_CENTRE = 'reduction.foreground.centre_workspace_index'
    FOREGROUND_END = 'reduction.foreground.last_workspace_index'
    FOREGROUND_START = 'reduction.foreground.first_workspace_index'
    LINE_POSITION = 'reduction.line_position'
    SLIT2WIDTH = 'reduction.slit2width'
    SLIT3WIDTH = 'reduction.slit3width'
    SUM_TYPE = 'reduction.foreground.summation_type'


class WSCleanup:
    """A class to manage intermediate workspace cleanup."""

    OFF = 'Cleanup OFF'
    ON = 'Cleanup ON'

    def __init__(self, cleanupMode, deleteAlgorithmLogging):
        """Initialize an instance of the class."""
        self._deleteAlgorithmLogging = deleteAlgorithmLogging
        self._doDelete = cleanupMode == self.ON
        self._protected = set()
        self._toBeDeleted = set()

    def cleanup(self, *args):
        """Delete the workspaces listed in *args."""
        for ws in args:
            self._delete(ws)

    def cleanupLater(self, *args):
        """Mark the workspaces listed in *args to be cleaned up later."""
        for arg in args:
            self._toBeDeleted.add(str(arg))

    def finalCleanup(self):
        """Delete all workspaces marked to be cleaned up later."""
        for ws in self._toBeDeleted:
            self._delete(ws)

    def protect(self, *args):
        """Mark the workspaces listed in *args to be never deleted."""
        for arg in args:
            self._protected.add(str(arg))

    def _delete(self, ws):
        """Delete the given workspace in ws if it is not protected, and
        deletion is actually turned on.
        """
        if not self._doDelete:
            return
        try:
            ws = str(ws)
        except RuntimeError:
            return
        if ws not in self._protected and mtd.doesExist(ws):
            DeleteWorkspace(Workspace=ws, EnableLogging=self._deleteAlgorithmLogging)


class WSNameSource:
    """A class to provide names for intermediate workspaces."""

    def __init__(self, prefix, cleanupMode):
        """Initialize an instance of the class."""
        self._names = set()
        self._prefix = '__' + prefix if cleanupMode == WSCleanup.ON else prefix

    def withSuffix(self, suffix):
        """Returns a workspace name with given suffix applied."""
        return self._prefix + '_' + suffix + '_'
