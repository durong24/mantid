# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

import getpass
import json
import os
import shutil
import sys
import tempfile
import time
import unittest

from mantid.api import AnalysisDataService as ADS
from mantid.kernel import ConfigService
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces
from mantidqt.project.recovery.projectrecovery import ProjectRecovery, SAVING_TIME_KEY, NO_OF_CHECKPOINTS_KEY, \
    RECOVERY_ENABLED_KEY

if sys.version_info.major >= 3:
    from unittest import mock
    unicode = str
else:
    import mock


class FakeEncoder(object):
    tags = ["FakeEncoder"]

    @staticmethod
    def encode(_, __):
        return {}


class ProjectRecoveryTest(unittest.TestCase):
    def setUp(self):
        self.globalfigmanager = mock.MagicMock()
        self.window_finder = mock.MagicMock()
        self.multifileinterpreter = mock.MagicMock()
        self.pr = ProjectRecovery(self.globalfigmanager, self.window_finder, self.multifileinterpreter)
        self.working_directory = tempfile.mkdtemp()

    def tearDown(self):
        ADS.clear()
        if os.path.exists(self.pr.recovery_directory_hostname):
            shutil.rmtree(self.pr.recovery_directory_hostname)

        if os.path.exists(self.working_directory):
            shutil.rmtree(self.working_directory)

    def test_constructor_settings_are_set(self):
        # Test the paths set in the constructor that are generated.
        self.assertEqual(self.pr.recovery_directory,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery"))
        self.assertEqual(self.pr.recovery_directory_hostname,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery", getpass.getuser()))
        self.assertEqual(self.pr.recovery_directory_pid,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery", getpass.getuser(),
                                      str(os.getpid())))
        self.assertEqual(self.pr.recovery_order_workspace_history_file,
                         os.path.join(ConfigService.getAppDataDirectory(), "ordered_recovery.py"))

        # Test config service values
        self.assertEqual(self.pr.time_between_saves, int(ConfigService[SAVING_TIME_KEY]))
        self.assertEqual(self.pr.maximum_num_checkpoints, int(ConfigService[NO_OF_CHECKPOINTS_KEY]))
        self.assertEqual(self.pr.recovery_enabled, ("true" == ConfigService[RECOVERY_ENABLED_KEY].lower()))

    def test_start_recovery_thread_if_thread_on_is_true(self):
        self.pr._timer_thread = mock.MagicMock()
        self.pr.thread_on = True
        self.pr.recovery_enabled = True

        self.pr.start_recovery_thread()

        self.assertEqual(self.pr._timer_thread.start.call_count, 0)

    def test_start_recovery_thread_if_thread_on_is_false(self):
        self.pr._timer_thread = mock.MagicMock()
        self.pr.thread_on = False
        self.pr.recovery_enabled = True

        self.pr.start_recovery_thread()

        self.assertEqual(self.pr._timer_thread.start.call_count, 1)

    def test_stop_recovery_thread(self):
        self.pr._timer_thread = mock.MagicMock()

        self.pr.stop_recovery_thread()

        self.assertEqual(self.pr._timer_thread.cancel.call_count, 1)

    def test_remove_empty_dir(self):
        self.assertTrue(os.path.exists(self.working_directory))

        # Feed parent to temp directory here, on Linux = '/tmp'
        parent_path, _ = os.path.split(self.working_directory)
        self.pr._remove_empty_folders_from_dir(parent_path)

        self.assertTrue(not os.path.exists(self.working_directory))

    def test_remove_all_folders_from_dir_raises_outside_of_mantid_dir(self):
        self.assertRaises(RuntimeError, self.pr._remove_directory_and_directory_trees, self.working_directory)

    def test_remove_all_folders_from_dir_doesnt_raise_inside_of_mantid_dir(self):
        temp_dir = os.path.join(self.pr.recovery_directory, "tempDir")
        os.mkdir(temp_dir)

        self.pr._remove_directory_and_directory_trees(temp_dir)

        self.assertTrue(not os.path.exists(temp_dir))

    def test_sort_paths_by_last_modified(self):
        # Make sure there is actually a different modified time on the files by using sleeps
        first = tempfile.mkdtemp()
        time.sleep(0.5)
        second = tempfile.mkdtemp()
        time.sleep(0.5)
        third = tempfile.mkdtemp()
        paths = [second, third, first]
        paths = self.pr.sort_by_last_modified(paths)

        self.assertListEqual(paths, [first, second, third])

    def test_get_pid_folder_to_be_used_to_load_a_checkpoint_from(self):
        # Needs to be a high number outside of possible pids
        one = os.path.join(self.pr.recovery_directory_hostname, "10000000")
        two = os.path.join(self.pr.recovery_directory_hostname, "20000000")

        # Make the first pid folder, neither pid will be in use but it should select the oldest first
        os.makedirs(one)
        time.sleep(0.01)
        os.makedirs(two)

        result = self.pr.get_pid_folder_to_load_a_checkpoint_from()
        self.assertEqual(one, result)

    def test_list_dir_full_path(self):
        one = os.path.join(self.working_directory, "10000000")
        two = os.path.join(self.working_directory, "20000000")
        os.makedirs(one)
        os.makedirs(two)

        # There is no concern for list order in this equality assertion
        if sys.version_info.major < 3:
            # Python 2.7 way of doing it
            self.assertItemsEqual([one, two], self.pr.listdir_fullpath(self.working_directory))
        else:
            # Python 3.2+ way of doing it
            self.assertCountEqual([one, two], self.pr.listdir_fullpath(self.working_directory))

    def test_recovery_save_when_nothing_is_present(self):
        self.pr._spin_off_another_time_thread = mock.MagicMock()
        self.assertIsNone(self.pr.recovery_save())

    def test_recovery_save_with_just_workspaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.pr._spin_off_another_time_thread = mock.MagicMock()

        # Do save
        self.pr.recovery_save()

        # Check 0.py was made
        checkpoint = self.pr.listdir_fullpath(self.pr.recovery_directory_pid)[0]
        self.assertTrue(os.path.exists(os.path.join(checkpoint, "0.py")))
        self.assertEqual(self.pr._spin_off_another_time_thread.call_count, 1)

    def test_recovery_save_with_just_interfaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.pr.interface_finding_func = mock.MagicMock(
            # Return a FakeEncoder object that will return an empty dictionary
            return_value=[[FakeEncoder(), FakeEncoder()]])
        self.pr._spin_off_another_time_thread = mock.MagicMock()
        ADS.clear()

        self.pr.recovery_save()

        # Check no 0.py was made
        checkpoint = self.pr.listdir_fullpath(self.pr.recovery_directory_pid)[0]
        self.assertTrue(not os.path.exists(os.path.join(checkpoint, "0.py")))
        self.assertEqual(self.pr._spin_off_another_time_thread.call_count, 1)

        # Read the .json file and check nothing is in workspace and something is in the interfaces dictionary
        with open(os.path.join(checkpoint, (os.path.basename(checkpoint) + self.pr.recovery_file_ext))) as f:
            dictionary = json.load(f)
            self.assertEqual(len(dictionary["interfaces"]), 1)
            self.assertEqual(len(dictionary["workspaces"]), 0)

    def test_recovery_save_with_both_workspace_and_interfaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.pr.interface_finding_func = mock.MagicMock(
            # Return a FakeEncoder object that will return an empty dictionary
            return_value=[[FakeEncoder(), FakeEncoder()]])
        self.pr._spin_off_another_time_thread = mock.MagicMock()

        self.pr.recovery_save()

        # Check 0.py was made
        checkpoint = self.pr.listdir_fullpath(self.pr.recovery_directory_pid)[0]
        self.assertTrue(os.path.exists(os.path.join(checkpoint, "0.py")))
        self.assertEqual(self.pr._spin_off_another_time_thread.call_count, 1)

        # Read the .json file and check nothing is in workspace and something is in the interfaces dictionary
        with open(os.path.join(checkpoint, (os.path.basename(checkpoint) + self.pr.recovery_file_ext))) as f:
            dictionary = json.load(f)
            self.assertEqual(len(dictionary["interfaces"]), 1)
            self.assertEqual(len(dictionary["workspaces"]), 1)

    @mock.patch('mantidqt.project.recovery.projectrecovery.Timer')
    def test_spin_of_another_thread(self, timer):
        self.pr._spin_off_another_time_thread()
        timer.assert_has_calls([mock.call().start()])

    def test_save_workspaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")

        self.pr._save_workspaces(self.working_directory)

        # Assert that the 0.py and 1.py that are expected are made
        self.assertTrue(os.path.exists(os.path.join(self.working_directory, "0.py")))
        self.assertTrue(os.path.exists(os.path.join(self.working_directory, "1.py")))

    def test_empty_group_workspaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        CreateSampleWorkspace(OutputWorkspace="ws1")
        GroupWorkspaces(OutputWorkspace="Group", InputWorkspaces="ws,ws1")
        group_workspace = ADS.retrieve("Group")
        group_workspace.remove("ws")
        group_workspace.remove("ws1")

        self.assertTrue(self.pr._empty_group_workspace(group_workspace))

    def test_save_project(self):
        self.pr._save_project(self.working_directory)

        project_file = os.path.join(self.working_directory, (os.path.basename(self.working_directory) + self.pr.recovery_file_ext))
        self.assertTrue(os.path.exists(project_file))
        with open(project_file) as f:
            dictionary = json.load(f)
            self.assertEqual(len(dictionary["interfaces"]), 0)
            self.assertEqual(len(dictionary["workspaces"]), 0)

    def test_add_lock_file(self):
        self.pr._add_lock_file(self.working_directory)
        self.assertTrue(os.path.exists(os.path.join(self.working_directory, self.pr.lock_file_name)))

    def test_remove_lock_file(self):
        self.pr._add_lock_file(self.working_directory)
        self.assertTrue(os.path.exists(os.path.join(self.working_directory, self.pr.lock_file_name)))

        self.pr._remove_lock_file(self.working_directory)
        self.assertTrue(not os.path.exists(os.path.join(self.working_directory, self.pr.lock_file_name)))

    def test_check_for_recovery_where_no_checkpoints_exist(self):
        self.assertTrue(not self.pr.check_for_recover_checkpoint())

    @staticmethod
    def create_checkpoints_at_directory(directory, number):
        for ii in range(0, number):
            os.makedirs(os.path.join(directory, (str(ii) + "00000000"), "spare_dir"))

    def test_check_for_recovery_when_2_instances_exist_with_2_checkpoints(self):
        # Doesn't do recovery so returns false

        # Create the 2 checkpoints
        self.create_checkpoints_at_directory(self.pr.recovery_directory_hostname, 2)
        self.pr._number_of_other_workbench_processes = mock.MagicMock(return_value=2)

        self.assertTrue(not self.pr.check_for_recover_checkpoint())

    def test_check_for_recovery_when_2_instances_exist_with_3_checkpoints(self):
        # Does recovery so returns true
        # Create the 3 checkpoints
        self.create_checkpoints_at_directory(self.pr.recovery_directory_hostname, 3)
        self.pr._number_of_other_workbench_processes = mock.MagicMock(return_value=2)

        self.assertTrue(self.pr.check_for_recover_checkpoint())

    @mock.patch('mantidqt.project.recovery.projectrecovery.ProjectRecoveryPresenter')
    def test_attempt_recovery_and_recovery_passes(self, presenter):
        presenter.return_value.start_recovery_view.return_value = True
        presenter.return_value.start_recovery_failure.return_value = True
        self.pr.clear_all_unused_checkpoints = mock.MagicMock()
        self.pr.start_recovery_thread = mock.MagicMock()

        self.pr.attempt_recovery()

        self.assertEqual(presenter.return_value.start_recovery_view.call_count, 1)
        self.assertEqual(presenter.return_value.start_recovery_failure.call_count, 0)
        self.assertEqual(self.pr.clear_all_unused_checkpoints.call_count, 1)
        self.assertEqual(self.pr.start_recovery_thread.call_count, 1)

    @mock.patch('mantidqt.project.recovery.projectrecovery.ProjectRecoveryPresenter')
    def test_attempt_recovery_and_recovery_fails_first_time_but_is_successful_on_failure_view(self, presenter):
        presenter.return_value.start_recovery_view.return_value = False
        presenter.return_value.start_recovery_failure.return_value = True
        self.pr.clear_all_unused_checkpoints = mock.MagicMock()
        self.pr.start_recovery_thread = mock.MagicMock()

        self.pr.attempt_recovery()

        self.assertEqual(presenter.return_value.start_recovery_view.call_count, 1)
        self.assertEqual(presenter.return_value.start_recovery_failure.call_count, 1)
        self.assertEqual(self.pr.clear_all_unused_checkpoints.call_count, 1)
        self.assertEqual(self.pr.start_recovery_thread.call_count, 1)

    def test_load_checkpoint(self):
        # Create the checkpoint
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.pr._spin_off_another_time_thread = mock.MagicMock()
        self.pr.recovery_save()

        # Mock out excess function calls
        self.pr.recovery_presenter = mock.MagicMock()
        self.pr._open_script_in_editor_call = mock.MagicMock()
        self.pr._run_script_in_open_editor = mock.MagicMock()

        # Find the checkpoint
        checkpoints = os.listdir(self.pr.recovery_directory_pid)
        checkpoint = os.path.join(self.pr.recovery_directory_pid, checkpoints[0])

        self.pr.load_checkpoint(checkpoint)

        # Test the calls are made properly
        self.assertEqual(self.pr._open_script_in_editor_call.call_count, 1)
        self.assertEqual(self.pr._run_script_in_open_editor.call_count, 1)

    @mock.patch('mantidqt.project.recovery.projectrecovery.ProjectLoader')
    def test_load_project_interfaces_call(self, loader):
        loader.return_value.load_project.return_value = True

        self.pr._load_project_interfaces("")

        self.assertEqual(loader.return_value.load_project.call_args, mock.call(directory='', load_workspaces=False))

    def test_compile_recovery_script(self):
        # make sure to clear out the script if it exists
        if os.path.exists(self.pr.recovery_order_workspace_history_file):
            os.remove(self.pr.recovery_order_workspace_history_file)

        # Create checkpoint
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.pr._spin_off_another_time_thread = mock.MagicMock()
        self.pr.recovery_save()

        # Find the checkpoint
        checkpoints = os.listdir(self.pr.recovery_directory_pid)
        checkpoint = os.path.join(self.pr.recovery_directory_pid, checkpoints[0])

        self.pr._compile_recovery_script(checkpoint)

        self.assertTrue(os.path.exists(self.pr.recovery_order_workspace_history_file))

        # Confirm contents is correct
        with open(self.pr.recovery_order_workspace_history_file, 'r') as f:
            actual_file_contents = f.read()

        file_contents = ""
        # Strip out the time
        for ii in actual_file_contents:
            if ii == '#':
                break
            file_contents += ii

        self.assertEqual(file_contents,
                         "from mantid.simpleapi import *\n\nCreateSampleWorkspace(OutputWorkspace='ws') ")

    def test_open_script_in_editor(self):
        self.pr.recovery_presenter = mock.MagicMock()
        self.pr._open_script_in_editor_call = mock.MagicMock()

        # Ensure a script file exists
        script = os.path.join(self.working_directory, "script")
        open(script, 'a').close()

        self.pr._open_script_in_editor(script)

        self.assertEqual(self.pr._open_script_in_editor_call.call_count, 1)
        self.assertEqual(self.pr.recovery_presenter.set_up_progress_bar.call_count, 1)
        self.assertEqual(self.pr.recovery_presenter.set_up_progress_bar.call_args, mock.call(0))

    def test_remove_oldest_checkpoints(self):
        self.pr.recovery_directory_pid = self.working_directory
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()

        for ii in range(0, self.pr.maximum_num_checkpoints+1):
            os.mkdir(os.path.join(self.working_directory, "dir"+str(ii)))
            time.sleep(0.01)

        self.pr.remove_oldest_checkpoints()

        # Now should have had a call made to delete working_directory + dir0
        self.pr._remove_directory_and_directory_trees.assert_called_with(os.path.join(self.working_directory, "dir0"))

    def test_clear_all_unused_checkpoints_called_with_none_and_only_one_user(self):
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()
        os.makedirs(self.pr.recovery_directory_hostname)

        self.pr.clear_all_unused_checkpoints()

        self.pr._remove_directory_and_directory_trees.assert_called_with(self.pr.recovery_directory_hostname)

    def test_clear_all_unused_checkpoints_called_with_none_and_multiple_users(self):
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()
        os.makedirs(self.pr.recovery_directory_hostname)
        user = os.path.join(self.pr.recovery_directory, "dimitar")
        if os.path.exists(user):
            shutil.rmtree(user)
        os.makedirs(user)

        self.pr.clear_all_unused_checkpoints()

        self.pr._remove_directory_and_directory_trees.assert_called_with(self.pr.recovery_directory_hostname)

    def test_clear_all_unused_checkpoints_called_with_not_none(self):
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()
        os.makedirs(self.pr.recovery_directory_hostname)

        self.pr.clear_all_unused_checkpoints(pid_dir=self.pr.recovery_directory_hostname)

        self.pr._remove_directory_and_directory_trees.assert_called_with(self.pr.recovery_directory_hostname)

    def _repair_checkpoint_checkpoints_setup(self, checkpoint1, checkpoint2, pid, pid2):
        if os.path.exists(pid):
            shutil.rmtree(pid)
        os.makedirs(pid)
        os.makedirs(checkpoint1)
        if os.path.exists(pid2):
            shutil.rmtree(pid2)
        os.makedirs(pid2)
        os.makedirs(checkpoint2)

        # Add a lock file to checkpoint 1
        open(os.path.join(checkpoint1, self.pr.lock_file_name), 'a').close()

        # Add one workspace to the checkpoint and change modified dates to older than a month
        os.utime(pid2, (1, 1))

    def _repair_checkpoints_assertions(self, checkpoint1, checkpoint2, pid, pid2):
        # None of the checkpoints should exist after the call. Thus the PID folder should be deleted and thus ignored.
        directory_removal_calls = [mock.call(os.path.join(self.pr.recovery_directory_hostname, '200000')),
                                   mock.call(os.path.join(self.pr.recovery_directory_hostname, "1000000", "check1"))]

        self.pr._remove_directory_and_directory_trees.assert_has_calls(directory_removal_calls)

        empty_file_calls = [mock.call(self.pr.recovery_directory_hostname)]
        self.pr._remove_empty_folders_from_dir.assert_has_calls(empty_file_calls)

        self.assertTrue(os.path.exists(checkpoint1))
        self.assertTrue(os.path.exists(pid2))
        self.assertTrue(os.path.exists(checkpoint2))
        self.assertTrue(os.path.exists(pid))

    def test_repair_checkpoints(self):
        pid = os.path.join(self.pr.recovery_directory_hostname, "1000000")
        checkpoint1 = os.path.join(pid, "check1")
        pid2 = os.path.join(self.pr.recovery_directory_hostname, "200000")
        checkpoint2 = os.path.join(pid, "check3")
        self._repair_checkpoint_checkpoints_setup(checkpoint1, checkpoint2, pid, pid2)
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()
        self.pr._remove_empty_folders_from_dir = mock.MagicMock()

        self.pr.repair_checkpoints()

        self._repair_checkpoints_assertions(checkpoint1, checkpoint2, pid, pid2)

        self.pr._remove_empty_folders_from_dir(self.pr.recovery_directory_hostname)

    def test_find_checkpoints_older_than_a_month(self):
        pid = os.path.join(self.pr.recovery_directory_hostname, "1000000")
        if os.path.exists(pid):
            shutil.rmtree(pid)

        os.makedirs(pid)
        os.utime(pid, (1, 1))

        self.assertEqual([pid], self.pr._find_checkpoints_older_than_a_month([pid]))

    def test_find_checkpoints_which_are_locked(self):
        pid = os.path.join(self.pr.recovery_directory_hostname, "1000000")
        if os.path.exists(pid):
            shutil.rmtree(pid)
        checkpoint1 = os.path.join(pid, "check1")

        os.makedirs(pid)
        os.makedirs(checkpoint1)
        # Add a lock file to checkpoint 1
        open(os.path.join(checkpoint1, self.pr.lock_file_name), 'a').close()

        self.assertEqual([checkpoint1], self.pr._find_checkpoints_which_are_locked([pid]))
