# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from unittest.mock import patch, MagicMock
from Engineering.gui.engineering_diffraction.tabs.focus import model, view, presenter
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo

tab_path = "Engineering.gui.engineering_diffraction.tabs.focus"


class FocusPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.FocusView)
        self.model = mock.create_autospec(model.FocusModel)
        self.presenter = presenter.FocusPresenter(self.model, self.view)
        self.presenter.cropping_widget = MagicMock()

    @patch(tab_path + ".presenter.check_workspaces_exist")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    def test_worker_started_with_correct_params(self, worker, wsp_exists):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path="Fake/Path",
                                                             sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.get_focus_filenames.return_value = "305738"
        self.presenter.cropping_widget.get_bank.return_value = "2"
        self.presenter.cropping_widget.is_custom.return_value = False
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        wsp_exists.return_value = True

        self.presenter.on_focus_clicked()
        worker.assert_called_with("305738", ["2"], True, None, None)

    @patch(tab_path + ".presenter.check_workspaces_exist")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    def test_worker_started_with_correct_params_custom_crop(self, worker, wsp_exists):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path="Fake/Path",
                                                             sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.get_focus_filenames.return_value = "305738"
        self.presenter.cropping_widget.get_custom_spectra.return_value = "2-45"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        wsp_exists.return_value = True

        self.presenter.on_focus_clicked()
        worker.assert_called_with("305738", None, True, None, "2-45")

    @patch(tab_path + ".presenter.FocusPresenter._validate")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    def test_worker_not_started_validate_fails(self, worker, valid):
        valid.return_value = False

        self.presenter.on_focus_clicked()
        worker.assert_not_called()

    def test_controls_disabled_disables_both(self):
        self.presenter.set_focus_controls_enabled(False)

        self.view.set_focus_button_enabled.assert_called_with(False)
        self.view.set_plot_output_enabled.assert_called_with(False)

    def test_controls_enabled_enables_both(self):
        self.presenter.set_focus_controls_enabled(True)

        self.view.set_focus_button_enabled.assert_called_with(True)
        self.view.set_plot_output_enabled.assert_called_with(True)

    @patch(tab_path + ".presenter.FocusPresenter.emit_enable_button_signal")
    def test_on_worker_error_enables_controls(self, emit):
        fail_info = 2024278

        self.presenter._on_worker_error(fail_info)

        self.assertEqual(1, emit.call_count)

    def test_get_both_banks(self):
        self.view.get_crop_checked.return_value = False
        self.assertEqual((["1", "2"], None), self.presenter._get_banks())

    def test_get_north_bank(self):
        self.presenter.cropping_widget.is_custom.return_value = False
        self.presenter.cropping_widget.get_bank.return_value = "1"

        self.assertEqual((["1"], None), self.presenter._get_banks())

    def test_get_south_bank(self):
        self.presenter.cropping_widget.is_custom.return_value = False
        self.presenter.cropping_widget.get_bank.return_value = "2"

        self.assertEqual((["2"], None), self.presenter._get_banks())

    def test_get_custom_spectra(self):
        self.presenter.cropping_widget.get_custom_spectra.return_value = "10-15"

        self.assertEqual((None, "10-15"), self.presenter._get_banks())

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_invalid_focus_path(self, error_message):
        self.view.get_focus_valid.return_value = False

        self.assertFalse(self.presenter._validate())
        self.assertEqual(error_message.call_count, 1)

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_invalid_calibration(self, create_error):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path=None,
                                                             sample_path=None,
                                                             instrument=None)
        self.view.is_searching.return_value = False

        self.presenter._validate()
        create_error.assert_called_with(
            self.presenter.view,
            "Create or Load a calibration via the Calibration tab before focusing.")

    @patch(tab_path + ".presenter.check_workspaces_exist")
    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_while_searching(self, create_error, wsp_check):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path="Fake/File/Path",
                                                             sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.is_searching.return_value = True
        wsp_check.return_value = True

        self.assertEqual(False, self.presenter._validate())
        self.assertEqual(1, create_error.call_count)

    @patch(tab_path + ".presenter.check_workspaces_exist")
    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_invalid_spectra(self, create_error, wsp_check):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path="Fake/Path",
                                                             sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.is_searching.return_value = False
        wsp_check.return_value = True
        self.presenter.cropping_widget.is_valid.return_value = False

        self.presenter._validate()
        create_error.assert_called_with(self.presenter.view, "Check cropping values are valid.")


if __name__ == '__main__':
    unittest.main()
