# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
#
#

from typing import List

from mantid.api import MatrixWorkspace, NumericAxis, Workspace, WorkspaceFactory
from mantid.plots.utility import get_single_workspace_log_value
from mantidqt.plotting.functions import pcolormesh

DEFAULT_LEVELS = [0.2, 0.4, 0.6, 0.8]

def plot(plot_type, plot_index, axis_name, log_name, custom_log_values, workspaces):
    if len(workspaces) > 0:
        matrix_ws = create_workspace_for_group_plot(plot_type, workspaces, plot_index, log_name, custom_log_values)

        title = "plot for {}, index {}".format([ws.name() for ws in workspaces], plot_index)

        if plot_type == "surface":
            import matplotlib.pyplot as plt
            fig, ax = plt.subplots(subplot_kw={'projection': 'mantid3d'})
            surface = ax.plot_surface(matrix_ws, cmap='viridis')
            ax.set_title("Surface " + title)
            ax.set_ylabel(axis_name)
            fig.show()
        elif plot_type == "contour":
            fig = pcolormesh([matrix_ws])
            ax = fig.get_axes()[0]

            array = fig.get_axes()[1].collections[0].get_array()
            levels = [array.max() * value for value in DEFAULT_LEVELS]

            ax.contour(matrix_ws, levels=levels, colors='k', linewidths=0.5)

            ax.set_ylabel(axis_name)
            ax.set_title("Contour " + title)

def create_workspace_for_group_plot\
    (plot_type: str, workspaces: List[Workspace], plot_index: int, log_name: str, custom_log_values: List[float]) -> MatrixWorkspace:
    validate_workspace_choices(workspaces, plot_index)

    number_of_workspaces = len(workspaces)

    first_ws = workspaces[0]
    first_blocksize = first_ws.blocksize()

    if plot_type == "contour":
        x_size = first_blocksize + 1
    else:
        x_size = first_blocksize

    matrix_ws = WorkspaceFactory.Instance().create(
        parent=first_ws, NVectors=number_of_workspaces, XLength=x_size, YLength=first_blocksize)

    matrix_ws.setYUnitLabel(first_ws.YUnitLabel())

    log_values = []
    for i in range(number_of_workspaces):
        ws = workspaces[i]
        if isinstance(ws, MatrixWorkspace):
            if plot_type == "contour":
                matrix_ws.applyBinEdgesFromAnotherWorkspace(ws, plot_index, i)
            else:
                matrix_ws.applyPointsFromAnotherWorkspace(ws, plot_index, i)

            matrix_ws.setY(i, ws.readY(plot_index))
            matrix_ws.setE(i, ws.readE(plot_index))

            if log_name == "Custom":
                log_values.append(get_single_workspace_log_value(i, log_values=custom_log_values))
            else:
                log_values.append(get_single_workspace_log_value(i, matrix_ws=ws, log_name=log_name))

    log_values_axis = NumericAxis.create(len(log_values))
    for i in range(len(log_values)):
        log_values_axis.setValue(i, log_values[i])

    matrix_ws.replaceAxis(1, log_values_axis)

    return matrix_ws


def group_contents_have_same_x(workspaces, index):
    # Check and retrieve X data for given workspace and spectrum
    def get_x_data(index, spectrum):
        nonlocal workspaces
        ws = workspaces[index]

        if isinstance(ws, MatrixWorkspace):
            if ws.getNumberHistograms() < spectrum:
                raise RuntimeError("Spectrum index too large for some workspaces.")
            else:
                return ws.readX(spectrum)
        else:
            raise RuntimeError("Group contains something other than MatrixWorkspaces.")

    number_of_workspaces = len(workspaces)
    if number_of_workspaces == 0:
        return False
    elif number_of_workspaces == 1:
        return True
    else:
        all_same_x = True
        first_x = get_x_data(0, index)
        for i in range(1, number_of_workspaces):
            x = get_x_data(i, index)
            if len(x) != len(first_x):
                all_same_x = False
                break

        return all_same_x


def validate_workspace_choices(workspaces, spectrum):
    if len(workspaces) == 0:
        raise RuntimeError("Must provide a non-empty WorkspaceGroup.")

    if not group_contents_have_same_x(workspaces, spectrum):
        raise RuntimeError("Input WorkspaceGroup must have same X data for all workspaces.")
