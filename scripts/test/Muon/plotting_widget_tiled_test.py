# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.plotting_widget.plotting_widget_presenter import PlotWidgetPresenter
from Muon.GUI.Common.contexts.fitting_context import FitInformation


@start_qapplication
class PlottingWidgetPresenterTestTiled(unittest.TestCase):
    def setUp(self):
        self.context = mock.MagicMock()
        self.context.fitting_context.number_of_fits = 1
        self.view = mock.MagicMock()
        self.model = mock.MagicMock()

        # set up presenter, view and context
        self.presenter = PlotWidgetPresenter(self.view, self.model, self.context)
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')
        self.view.is_tiled_plot = mock.MagicMock(return_value=True)
        self.context.data_context.instrument = "MUSR"
        axes = [mock.MagicMock(), mock.MagicMock, mock.MagicMock, mock.MagicMock]
        self.view.get_axes = mock.MagicMock(return_value=axes)

        # workspaces and corresponding tiled by group and run indices
        self.group_workspace_list = self.create_workspace_group_list()
        self.run_workspace_list = self.create_workspace_run_list()
        self.tiled_group = {'bottom': 0, 'top': 1, 'bkwd': 2, 'fwd': 3}
        self.tiled_runs = {'MUSR62260': 0, 'MUSR62261': 1, 'MUSR62262': 2, 'MUSR62263': 3,
                           'MUSR62264': 4, 'MUSR62265': 5, 'MUSR62266': 6, 'MUSR62267': 7}

        self.context.group_pair_context.selected_pairs = []
        self.context.group_pair_context.selected_groups = ['bottom', 'top', 'bkwd', 'fwd']
        self.context.data_context.current_runs = [['62260'], ['62261'], ['62262'], ['62263'],
                                                  ['62264'], ['62265'], ['62266'], ['62267']]
        self.view.plot_options.get_errors = mock.MagicMock(return_value=True)
        self.presenter.get_x_limits = mock.MagicMock(return_value=[0, 15])
        self.presenter.get_y_limits = mock.MagicMock(return_value=[0, 1])
        self.presenter.get_x_lim_from_subplot = mock.MagicMock(return_value=[0, 15])
        self.presenter.get_y_lim_from_subplot = mock.MagicMock(return_value=[-1, 1])

    def create_workspace_group_list(self):
        return ['MUSR62260; Group; bottom; Asymmetry; MA',
                'MUSR62260; Group; top; Asymmetry; MA',
                'MUSR62260; Group; bkwd; Asymmetry; MA',
                'MUSR62260; Group; fwd; Asymmetry; MA']

    def create_workspace_run_list(self):
        return ['MUSR62260; Group; bottom; Asymmetry; MA',
                'MUSR62261; Group; bottom; Asymmetry; MA',
                'MUSR62262; Group; bottom; Asymmetry; MA',
                'MUSR62263; Group; bottom; Asymmetry; MA',
                'MUSR62264; Group; bottom; Asymmetry; MA',
                'MUSR62265; Group; bottom; Asymmetry; MA',
                'MUSR62266; Group; bottom; Asymmetry; MA',
                'MUSR62267; Group; bottom; Asymmetry; MA']

    def create_simulatenous_fit_over_groups(self, workspace_list):
        output_workspaces = [ws + '; ' + 'Fitted;' for ws in workspace_list]
        fit_information = FitInformation(mock.MagicMock(),
                                         'GaussOsc',
                                         workspace_list,
                                         output_workspaces)

        return fit_information

    def create_axis(self, num_axes):
        axes = [None] * num_axes
        for i in range(num_axes):
            axes[i] = mock.MagicMock()
        self.view.get_axes.return_value = axes
        self.model.number_of_axes = num_axes

    def test_handle_tiled_plot_by_group_creates_figure_correctly(self):
        self.view.get_tiled_by_type.return_value = 'group'
        self.model.tiled_plot_positions = {}

        self.presenter.handle_plot_tiled_changed(2)

        self.view.new_plot_figure.assert_called_with(len(self.tiled_group))

    def test_handle_tiled_plot_by_run_creates_figure_correctly(self):
        self.view.get_tiled_by_type.return_value = 'run'
        self.model.tiled_plot_positions = {}

        self.presenter.handle_plot_tiled_changed(2)

        self.view.new_plot_figure.assert_called_with(len(self.tiled_runs))

    def test_update_model_tile_plot_positions_updates_tiled_positions_correctly(self):
        self.model.tiled_plot_positions = {}
        self.view.get_tiled_by_type.return_value = 'run'
        self.presenter.update_model_tile_plot_positions()

        self.assertEqual(self.model.tiled_plot_positions, self.tiled_runs)

        self.model.tiled_plot_positions = {}
        self.view.get_tiled_by_type.return_value = 'group'
        self.presenter.update_model_tile_plot_positions()

        self.assertEqual(self.model.tiled_plot_positions, self.tiled_group)

    def test_workspace_plot_axis_returns_correctly(self):
        # test tiled by group
        self.create_axis(len(self.tiled_group))
        self.view.get_tiled_by_type.return_value = 'group'
        self.model.tiled_plot_positions = self.tiled_group
        for i, workspace_name in enumerate(self.group_workspace_list):
            ax = self.presenter.get_workspace_plot_axis(workspace_name)
            self.assertEqual(ax, self.view.get_axes()[i])

        # test tiled by run
        self.create_axis(len(self.tiled_runs))
        self.view.get_tiled_by_type.return_value = 'run'
        self.model.tiled_plot_positions = self.tiled_runs
        for i, workspace_name in enumerate(self.run_workspace_list):
            ax = self.presenter.get_workspace_plot_axis(workspace_name)
            self.assertEqual(ax, self.view.get_axes()[i])

    def test_handle_workspace_replaced_calls_model_with_correct_axis(self):
        self.create_axis(len(self.tiled_group))
        replace_index = 2
        workspace_list = self.group_workspace_list
        self.model.plotted_workspaces = workspace_list
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=workspace_list)
        self.model.tiled_plot_positions = self.tiled_group
        self.view.get_tiled_by_type.return_value = 'group'

        self.presenter.handle_workspace_replaced_in_ads(workspace_list[replace_index])

        self.model.replace_workspace_plot.assert_called_once_with(
            workspace_list[replace_index],
            self.view.get_axes()[replace_index])

    def test_handle_plot_guess_changed_adds_guess_to_all_plots(self):
        self.create_axis(len(self.tiled_group))
        self.context.fitting_context.plot_guess = True
        self.context.fitting_context.guess_ws = 'ws_guess'
        self.model.plotted_fit_workspaces = []
        self.model.tiled_plot_positions = self.tiled_group
        self.view.get_tiled_by_type.return_value = 'group'
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'Fit Function Guess'}

        self.presenter.handle_plot_guess_changed()

        self.assertEqual(self.model.number_of_axes, self.model.add_workspace_to_plot.call_count)
        for i in range(self.model.number_of_axes):
            self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[i], 'ws_guess',
                                                             workspace_indices=[1], errors=False,
                                                             plot_kwargs=plot_kwargs)

    def test_handle_plot_guess_changed_removes_all_guesses_if_workspace_is_none(self):
        self.create_axis(len(self.tiled_group))
        self.context.fitting_context.plot_guess = True
        self.context.fitting_context.guess_ws = None
        self.model.plotted_fit_workspaces = ['ws1', 'ws2_guess', 'ws3_guess', 'ws4']
        self.model.tiled_plot_positions = self.tiled_group
        self.view.get_tiled_by_type.return_value = 'group'

        self.presenter.handle_plot_guess_changed()

        call_list = [mock.call('ws2_guess', self.view.get_axes()), mock.call('ws3_guess', self.view.get_axes())]

        self.assertEqual(0, self.model.add_workspace_to_plot.call_count)
        self.assertEqual(2, self.model.remove_workspace_from_plot.call_count)
        self.model.remove_workspace_from_plot.assert_has_calls(call_list)

    def test_add_workspace_called_with_correct_axis_by_handle_data_updated_if_run_list_changed(self):
        self.create_axis(len(self.tiled_group))
        self.model.plotted_workspaces = []
        workspace_list = self.group_workspace_list
        self.model.tiled_plot_positions = self.tiled_group
        self.view.get_tiled_by_type.return_value = 'group'
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=workspace_list)
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='label')
        workspace_indices = [0]
        errors = True
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'label'}

        self.presenter.handle_data_updated()

        # check each workspace was plotted to the correct axis
        self.assertEqual(self.model.add_workspace_to_plot.call_count, self.model.number_of_axes)

        for i, ws in enumerate(workspace_list):
            self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[i],
                                                             ws,
                                                             workspace_indices,
                                                             errors=errors,
                                                             plot_kwargs=plot_kwargs)

    def test_handle_plot_type_changed_calls_add_workspace_to_plot_with_correct_axis(self):
        self.create_axis(len(self.tiled_group))
        self.model.plotted_workspaces = []
        workspace_list = self.group_workspace_list
        self.model.tiled_plot_positions = self.tiled_group
        self.view.get_tiled_by_type.return_value = 'group'
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=workspace_list)
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='label')
        workspace_indices = [0]
        errors = True
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'label'}

        self.presenter.handle_plot_type_changed()

        # check each workspace was plotted to the correct axis
        self.assertEqual(self.model.add_workspace_to_plot.call_count, self.model.number_of_axes)

        for i, ws in enumerate(workspace_list):
            self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[i],
                                                             ws,
                                                             workspace_indices,
                                                             errors=errors,
                                                             plot_kwargs=plot_kwargs)

    def test_handle_added_group_or_pair_to_plot_creates_new_figure_when_tiled_by_group(self):
        self.model.tiled_plot_positions = self.tiled_group
        self.view.get_tiled_by_type.return_value = 'group'

        self.presenter.handle_added_or_removed_group_or_pair_to_plot(is_added=True)

        self.view.new_plot_figure.assert_called_with(len(self.tiled_group))

    def test_handle_added_group_or_pair_to_plot_does_not_create_new_figure_if_tiled_by_run(self):
        self.model.tiled_plot_positions = self.tiled_runs
        self.view.get_tiled_by_type.return_value = 'run'

        self.presenter.handle_added_or_removed_group_or_pair_to_plot(is_added=True)

        self.view.new_plot_figure.assert_not_called()

    def test_handle_added_group_or_pair_to_plot_plots_correctly_when_tiled_by_group(self):
        self.create_axis(len(self.group_workspace_list))
        self.model.plotted_workspaces = self.group_workspace_list[0:2]
        self.model.tiled_plot_positions = {}
        self.view.get_tiled_by_type.return_value = 'group'
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=self.group_workspace_list)
        workspace_indices = [0]
        errors = True
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'MUSR62260'}

        self.presenter.handle_added_or_removed_group_or_pair_to_plot(is_added=True)

        # should be 2 plot calls, and make sure they were plotted to the correct axis
        self.assertEqual(self.model.add_workspace_to_plot.call_count, 2)
        self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[2],
                                                         self.group_workspace_list[2],
                                                         workspace_indices,
                                                         errors=errors,
                                                         plot_kwargs=plot_kwargs)

        self.model.add_workspace_to_plot.assert_called_with(self.view.get_axes()[3],
                                                            self.group_workspace_list[3],
                                                            workspace_indices,
                                                            errors=errors,
                                                            plot_kwargs=plot_kwargs)

    def test_removed_group_or_pair_from_plot_creates_new_figure_when_tiled_by_group(self):
        self.model.tiled_plot_positions = self.tiled_group
        self.view.get_tiled_by_type.return_value = 'group'

        self.presenter.handle_added_or_removed_group_or_pair_to_plot(is_added=False)

        self.view.new_plot_figure.assert_called_with(len(self.tiled_group))

    def test_handle_fit_completed_adds_appropriate_fits_to_tiles_when_fitting_over_groups(self):
        self.model.plotted_workspaces = self.group_workspace_list
        self.model.tiled_plot_positions = self.tiled_group
        self.create_axis(len(self.group_workspace_list))
        self.view.get_tiled_by_type.return_value = 'group'
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='label')
        fit_information = self.create_simulatenous_fit_over_groups(self.group_workspace_list)
        self.context.fitting_context.fit_list.__getitem__.return_value = fit_information
        self.context.fitting_context.number_of_fits = 1

        self.presenter.handle_fit_completed(fit_information)

        # 2 fit workspaces per plotted workspace
        self.assertEqual(self.model.add_workspace_to_plot.call_count, 2 * len(self.group_workspace_list))
        # check fit and diff workspaces plotted in each tile
        for i, ws in enumerate(self.group_workspace_list):
            self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[i],
                                                             fit_information.output_workspace_names[i], [1],
                                                             errors=False, plot_kwargs=mock.ANY)
            self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[i],
                                                             fit_information.output_workspace_names[i], [2],
                                                             errors=False, plot_kwargs=mock.ANY)

    def test_handle_subplot_changed_in_options_retrieves_correct_axis_limits(self):
        subplot = '3'
        index = int(subplot) - 1
        xlims = [0, 10]
        ylims = [-2, 2]
        self.view.plot_options.get_selection.return_value = [subplot]
        self.presenter.get_x_lim_from_subplot = mock.MagicMock(return_value=xlims)
        self.presenter.get_y_lim_from_subplot = mock.MagicMock(return_value=ylims)

        self.presenter.handle_subplot_changed_in_options()

        self.presenter.get_x_lim_from_subplot.assert_called_with(index)
        self.presenter.get_y_lim_from_subplot.assert_called_with(index)

    def test_handle_x_lims_changed_in_figure_view_updates_axis_if_subplot_selected_in_options(self):
        subplot = '3'
        index = int(subplot) - 1
        xlims = [0, 10]
        axis = self.view.get_axes()[index]
        axis.get_xlim = mock.MagicMock(return_value=xlims)
        self.view.plot_options.get_selection.return_value = [subplot]

        self.presenter.handle_x_axis_limits_changed_in_figure_view(axis)

        self.view.plot_options.set_plot_x_range.assert_called_once_with(xlims)

    def test_handle_x_lims_changed_in_figure_view_does_not_update_if_subplot_not_selected(self):
        subplot = '3'
        axis = self.view.get_axes()[0]
        self.view.plot_options.get_selection.return_value = [subplot]

        self.presenter.handle_x_axis_limits_changed_in_figure_view(axis)

        self.view.plot_options.set_plot_x_range.assert_not_called()

    def test_handle_y_lims_changed_in_figure_view_updates_axis_if_subplot_selected_in_options(self):
        subplot = '3'
        index = int(subplot) - 1
        ylims = [-2, 2]
        axis = self.view.get_axes()[index]
        axis.get_ylim = mock.MagicMock(return_value=ylims)
        self.view.plot_options.get_selection.return_value = [subplot]

        self.presenter.handle_y_axis_limits_changed_in_figure_view(axis)

        self.view.plot_options.set_plot_y_range.assert_called_once_with(ylims)

    def test_handle_y_lims_changed_in_figure_view_does_not_update_if_correct_subplot_not_selected(self):
        subplot = '3'
        axis = self.view.get_axes()[0]
        self.view.plot_options.get_selection.return_value = [subplot]

        self.presenter.handle_y_axis_limits_changed_in_figure_view(axis)

        self.view.plot_options.set_plot_y_range.assert_not_called()

    def test_handle_changed_to_tiled_plots_replots_previous_workspaces(self):
        workspace_list = self.group_workspace_list
        self.model.plotted_workspaces = workspace_list
        self.model.plotted_fit_workspaces = []
        self.presenter.plot_data_and_fit_workspaces = mock.MagicMock()

        self.presenter.handle_plot_tiled_changed(state=2)

        self.presenter.plot_data_and_fit_workspaces.assert_called_with(workspace_list, [])

    def test_handle_changed_tiled_plot_type_replots_previous_workspaces(self):
        workspace_list = self.group_workspace_list
        self.model.plotted_workspaces = workspace_list
        self.model.plotted_fit_workspaces = []
        self.presenter.plot_data_and_fit_workspaces = mock.MagicMock()

        self.presenter.handle_tiled_by_changed_on_view(1)

        self.presenter.plot_data_and_fit_workspaces.assert_called_with(workspace_list, [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
