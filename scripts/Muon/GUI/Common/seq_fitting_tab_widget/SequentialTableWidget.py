# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy.QtCore import Qt
from Muon.GUI.Common.seq_fitting_tab_widget.SequentialTableView import SequentialTableView
from Muon.GUI.Common.seq_fitting_tab_widget.SequentialTableModel import (SequentialTableModel, FIT_STATUS_COLUMN)
from Muon.GUI.Common.seq_fitting_tab_widget.SequentialTableDelegates import FitQualityDelegate, FIT_STATUSES
from collections import namedtuple

WorkspaceInfo = namedtuple('Workspace', 'runs groups')


class SequentialTableWidget(object):

    def __init__(self, parent):
        self._view = SequentialTableView(parent)
        self._model = SequentialTableModel()
        self._view.setModel(self._model)
        self._view.setItemDelegateForColumn(FIT_STATUS_COLUMN, FitQualityDelegate(self._view))
        self._view.resizeColumnsToContents()
        self._view.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)

    @property
    def widget(self):
        return self._view

    def block_signals(self, state):
        self._view.blockSignals(state)

    def get_number_of_fits(self):
        return self._model.number_of_fits

    def set_parameters_and_values(self, parameters, values=None):
        self.block_signals(True)
        self._model.set_fit_parameters_and_values(parameters, values)
        self.block_signals(False)
        self._view.resizeColumnsToContents()

    def set_parameter_values_for_row(self, row, parameters):
        self.block_signals(True)
        self._model.set_fit_parameter_values_for_row(row, parameters)
        self.block_signals(False)
        self._view.resizeColumnsToContents()

    def set_fit_quality(self, row, quality, chi_squared):
        self.block_signals(True)
        quality = self.get_shortened_fit_status(quality)
        self._model.set_fit_quality(row, quality, chi_squared)
        self.block_signals(False)

    def set_fit_workspaces(self, runs, group_and_pairs):
        self.block_signals(True)
        self._model.set_fit_workspaces(runs, group_and_pairs)
        self.block_signals(False)

    def set_selection_to_last_row(self):
        self._view.set_selection_to_last_row()

    def reset_fit_quality(self):
        self._model.reset_fit_quality()

    def clear_fit_parameters(self):
        self._model.clear_fit_parameters()

    def clear_fit_workspaces(self):
        self._model.clear_fit_workspaces()

    def clear_fit_selection(self):
        self._view.clearSelection()

    def get_selected_rows(self):
        rowSelectionModels = self._view.selectionModel().selectedRows()
        rows = []
        for model in rowSelectionModels:
            rows += [model.row()]
        return rows

    def get_workspace_info_from_row(self, row):
        if row > self._model.rowCount():
            return WorkspaceInfo([], [])

        run_numbers = self.get_runs_from_row(row)
        group_and_pairs = self.get_groups_and_pairs_from_row(row)
        return WorkspaceInfo(run_numbers, group_and_pairs)

    def get_runs_from_row(self, row):
        return self._model.get_run_information(row)

    def get_groups_and_pairs_from_row(self, row):
        return self._model.get_group_information(row)

    def get_fit_quality_from_row(self, row):
        return self._model.get_fit_quality(row)

    def get_fit_parameter_values_from_row(self, row):
        return self._model.get_fit_parameters(row)

    def setup_slot_for_row_selection_changed(self, slot):
        self._view.clicked.connect(slot)

    def set_slot_for_parameter_changed(self, slot):
        self._model.parameterChanged.connect(slot)

    def set_slot_for_key_up_down_pressed(self, slot):
        self._view.keyUpDownPressed.connect(slot)

    @staticmethod
    def get_shortened_fit_status(fit_status):
        accepted_statuses = list(FIT_STATUSES.keys())
        if "Failed" in fit_status:
            return accepted_statuses[3]
        elif "Changes" in fit_status:
            return accepted_statuses[2]
        elif "success" in fit_status:
            return accepted_statuses[1]
        else:
            return accepted_statuses[0]
