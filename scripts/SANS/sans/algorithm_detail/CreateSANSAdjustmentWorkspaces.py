# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

""" SANSCreateAdjustmentWorkspaces algorithm creates workspaces for pixel adjustment
    , wavelength adjustment and pixel-and-wavelength adjustment workspaces.
"""

from __future__ import (absolute_import, division, print_function)

from sans.algorithm_detail.CalculateSANSTransmission import CalculateSANSTransmission
from sans.algorithm_detail.NormalizeToSANSMonitor import NormalizeToSANSMonitor
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_unmanaged_algorithm


class CreateSANSAdjustmentWorkspaces(object, ):
    def __init__(self, state, component, data_type, slice_event_factor=1.0):
        """
        Calculates wavelength adjustment, pixel adjustment workspaces and wavelength-and-pixel adjustment workspaces.
        :param state: The SANS State object
        :param component: The component of the instrument which is currently being investigated.
                          Allowed values: ['HAB', 'LAB']
        :param data_type: The component of the instrument which is to be reduced.
                          Allowed values: ['Sample', 'Can']
        :param slice_event_factor: (Optional) The slice factor for the monitor normalization.
                                   This factor is the one obtained from event slicing.
        """
        self._component = component
        self._data_type = data_type
        self._slice_event_factor = slice_event_factor
        self._state = state

    def create_sans_adjustment_workspaces(self, transmission_ws, direct_ws, monitor_ws, sample_data):
        """
        Creates the adjustment workspace
        :param transmission_ws: The transmission workspace.
        :param direct_ws: The direct workspace.
        :param monitor_ws: The scatter monitor workspace. This workspace only contains monitors.
        :param sample_data: A workspace cropped to the detector to be reduced (the SAME as the input to Q1D).
                            This used to verify the solid angle. The workspace is not modified, just inspected.
        :return: A dict containing the following:
                 wavelength_adj : The workspace for wavelength-based adjustments.
                 pixel_adj : The workspace for wavelength-based adjustments.
                 wavelength_pixel_adj : The workspace for, both, wavelength- and pixel-based adjustments.
                 calculated_trans_ws : The calculated transmission workspace
                 unfitted_trans_ws : The unfitted transmission workspace
        """
        # Read the state

        # --------------------------------------
        # Get the monitor normalization workspace
        # --------------------------------------
        monitor_normalization_workspace = self._get_monitor_normalization_workspace(monitor_ws=monitor_ws)

        # --------------------------------------
        # Get the calculated transmission
        # --------------------------------------
        calculated_trans_ws, unfitted_transmission_workspace = \
            self._get_calculated_transmission_workspace(direct_ws=direct_ws, transmission_ws=transmission_ws)

        # --------------------------------------
        # Get the wide angle correction workspace
        # --------------------------------------
        wavelength_and_pixel_adj_workspace = \
            self._get_wide_angle_correction_workspace(sample_data=sample_data,
                                                      calculated_transmission_workspace=calculated_trans_ws)

        # --------------------------------------------
        # Get the full wavelength and pixel adjustment
        # --------------------------------------------
        wavelength_adjustment_workspace, pixel_length_adjustment_workspace = \
            self._get_wavelength_and_pixel_adjustment_workspaces(
                calculated_transmission_workspace=calculated_trans_ws,
                monitor_normalization_workspace=monitor_normalization_workspace)

        to_return = {"wavelength_adj": wavelength_adjustment_workspace,
                     "pixel_adj": pixel_length_adjustment_workspace,
                     "wavelength_pixel_adj": wavelength_and_pixel_adj_workspace,
                     "calculated_trans_ws": calculated_trans_ws,
                     "unfitted_trans_ws": unfitted_transmission_workspace}

        return to_return

    def _get_wavelength_and_pixel_adjustment_workspaces(self,
                                                        monitor_normalization_workspace,
                                                        calculated_transmission_workspace):
        component = self._component

        # TODO change this to use update CreateWavelengthAndPixelAdj
        # TODO Do NOT Merge till this is done

        wave_pixel_adjustment_name = "SANSCreateWavelengthAndPixelAdjustment"
        serialized_state = self._state.property_manager
        wave_pixel_adjustment_options = {"SANSState": serialized_state,
                                         "NormalizeToMonitorWorkspace": monitor_normalization_workspace,
                                         "OutputWorkspaceWavelengthAdjustment": EMPTY_NAME,
                                         "OutputWorkspacePixelAdjustment": EMPTY_NAME,
                                         "Component": component}
        if calculated_transmission_workspace:
            wave_pixel_adjustment_options.update({"TransmissionWorkspace": calculated_transmission_workspace})
        wave_pixel_adjustment_alg = create_unmanaged_algorithm(wave_pixel_adjustment_name,
                                                               **wave_pixel_adjustment_options)

        wave_pixel_adjustment_alg.execute()
        wavelength_out = wave_pixel_adjustment_alg.getProperty("OutputWorkspaceWavelengthAdjustment").value
        pixel_out = wave_pixel_adjustment_alg.getProperty("OutputWorkspacePixelAdjustment").value
        return wavelength_out, pixel_out

    def _get_monitor_normalization_workspace(self, monitor_ws):
        state = self._state
        scale_factor = self._slice_event_factor

        alg = NormalizeToSANSMonitor(state_adjustment_normalize_to_monitor=state.adjustment.normalize_to_monitor)
        ws = alg.normalize_to_monitor(workspace=monitor_ws, scale_factor=scale_factor)

        return ws

    def _get_calculated_transmission_workspace(self, transmission_ws, direct_ws):
        """
        Creates the fitted transmission workspace.
        Note that this step is not mandatory. If no transmission and direct workspaces are provided, then we
        don't have to do anything here.
        """

        fitted_data, unfitted_data = None, None

        if transmission_ws and direct_ws:
            data_type = self._data_type
            calc_trans_state = self._state.adjustment.calculate_transmission
            alg = CalculateSANSTransmission(data_type_str=data_type,
                                            state_adjustment_calculate_transmission=calc_trans_state)

            fitted_data, unfitted_data = alg.calculate_transmission(transmission_ws=transmission_ws,
                                                                    direct_ws=direct_ws)
        return fitted_data, unfitted_data

    def _get_wide_angle_correction_workspace(self, sample_data, calculated_transmission_workspace):
        wide_angle_correction = self._state.adjustment.wide_angle_correction

        workspace = None
        if wide_angle_correction and sample_data and calculated_transmission_workspace:
            wide_angle_name = "SANSWideAngleCorrection"
            wide_angle_options = {"SampleData": sample_data,
                                  "TransmissionData": calculated_transmission_workspace,
                                  "OutputWorkspace": EMPTY_NAME}
            wide_angle_alg = create_unmanaged_algorithm(wide_angle_name, **wide_angle_options)
            wide_angle_alg.execute()
            workspace = wide_angle_alg.getProperty("OutputWorkspace").value
        return workspace
