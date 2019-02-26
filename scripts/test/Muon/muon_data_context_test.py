# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from Muon.GUI.Common.muon_data_context import MuonDataContext
from mantid.api import AnalysisDataService
import unittest

from mantid.api import FileFinder

import copy

if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock


class MuonDataContextTest(unittest.TestCase):
    def setUp(self):
        self.loaded_data = MuonLoadData()
        self.context = MuonDataContext(self.loaded_data)
        self.context.instrument = 'EMU'

        filepath = FileFinder.findRuns('EMU00019489.nxs')[0]

        load_result, run, filename = load_workspace_from_filename(filepath)

        self.loaded_data.add_data(workspace=load_result, run=[run], filename=filename, instrument='EMU')
        self.context.current_runs = [[run]]
        self.context.update_current_data()

    def tearDown(self):
        AnalysisDataService.clear()

    def test_that_setting_a_fixed_rebin_step_calculates_all_groups_and_pairs_twice(self):
        self.context.gui_variables['RebinType'] = 'Fixed'
        self.context.gui_variables['RebinFixed'] = '2'

        self.context.show_all_groups()

        expected_workspaces = ['EMU19489', 'EMU19489 Groups', 'EMU19489; Group; bwd; Asymmetry; #1', 'EMU19489; Group; bwd; Asymmetry; Rebin; #1',
                               'EMU19489; Group; bwd; Counts; #1', 'EMU19489; Group; bwd; Counts; Rebin; #1', 'EMU19489; Group; fwd; Asymmetry; #1',
                               'EMU19489; Group; fwd; Asymmetry; Rebin; #1', 'EMU19489; Group; fwd; Counts; #1', 'EMU19489; Group; fwd; Counts; Rebin; #1',
                               'Muon Data']


        self.assertEqual(AnalysisDataService.getObjectNames(), expected_workspaces)

    def test_that_setting_no_rebinning_calculates_groups_and_pairs_once(self):
        self.context.gui_variables['RebinType'] = 'None'
        self.context.show_all_groups()

        expected_workspaces = ['EMU19489', 'EMU19489 Groups',
                               'EMU19489; Group; bwd; Asymmetry; #1',
                               'EMU19489; Group; bwd; Counts; #1',
                               'EMU19489; Group; fwd; Asymmetry; #1',
                               'EMU19489; Group; fwd; Counts; #1',
                               'Muon Data']

        self.assertEqual(AnalysisDataService.getObjectNames(), expected_workspaces)

    def test_that_setting_a_variable_rebin_step_calculates_all_groups_and_pairs_twice(self):
        self.context.gui_variables['RebinType'] = 'Variable'
        self.context.gui_variables['RebinVariable'] = '1,0.1,2'

        self.context.show_all_groups()

        expected_workspaces = ['EMU19489', 'EMU19489 Groups', 'EMU19489; Group; bwd; Asymmetry; #1', 'EMU19489; Group; bwd; Asymmetry; Rebin; #1',
                               'EMU19489; Group; bwd; Counts; #1', 'EMU19489; Group; bwd; Counts; Rebin; #1', 'EMU19489; Group; fwd; Asymmetry; #1',
                               'EMU19489; Group; fwd; Asymmetry; Rebin; #1', 'EMU19489; Group; fwd; Counts; #1', 'EMU19489; Group; fwd; Counts; Rebin; #1',
                               'Muon Data']


        self.assertEqual(AnalysisDataService.getObjectNames(), expected_workspaces)

    def test_setting_current_data_with_a_different_field_sends_message_signal(self):
        self.context.current_data['MainFieldDirection'] = 'transverse'
        self.context.message_notifier.notify_subscribers = mock.MagicMock()

        self.context.update_current_data()

        self.context.message_notifier.notify_subscribers.assert_called_once_with('MainFieldDirection has changed between'
                                                                                 ' data sets, click default to reset grouping if required')

    def test_that_setting_current_runs_with_mixture_of_transverse_and_longitudanal_runs_raises_warning(self):
        loaded_data = copy.copy(self.context.current_data)
        self.context.message_notifier.notify_subscribers = mock.MagicMock()
        loaded_data['MainFieldDirection'] = 'transverse'

        self.loaded_data.add_data(workspace=loaded_data, run=[1], filename='filename', instrument='EMU')

        self.context.current_runs = [[19489], [1]]

        self.context.message_notifier.notify_subscribers.assert_called_once_with(
            'MainFieldDirection changes within current run set')


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)