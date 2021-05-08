# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import traceback

from qtpy.QtCore import Slot, QThreadPool, Signal, QObject

from mantid.kernel import Logger
from sans.algorithm_detail.batch_execution import load_workspaces_from_states
from sans.common.enums import ReductionMode
from sans.sans_batch import SANSBatchReduction
from ui.sans_isis.worker import Worker


class BatchProcessRunner(QObject):
    row_processed_signal = Signal(int, list, list)
    row_failed_signal = Signal(int, str)

    def __init__(self, notify_progress, notify_done, notify_error):
        super(BatchProcessRunner, self).__init__()
        self.row_processed_signal.connect(notify_progress)
        self.row_failed_signal.connect(notify_error)
        self.notify_done = notify_done
        self.batch_processor = SANSBatchReduction()
        self._logger = Logger("SANS")
        self._worker = None

    @Slot()
    def on_finished(self):
        result = self._worker.result if self._worker else None
        self._worker = None
        self.notify_done(result)

    @Slot()
    def on_error(self):
        self._worker = None

    def process_states(self, row_index_pair, get_states_func, use_optimizations, output_mode, plot_results, output_graph,
                       save_can=False):
        self._worker = Worker(self._process_states_on_thread,
                              row_index_pair=row_index_pair, get_states_func=get_states_func, use_optimizations=use_optimizations,
                              output_mode=output_mode, plot_results=plot_results,
                              output_graph=output_graph, save_can=save_can)
        self._worker.signals.finished.connect(self.on_finished)
        self._worker.signals.error.connect(self.on_error)

        QThreadPool.globalInstance().start(self._worker)

    def load_workspaces(self, row_index_pair, get_states_func):

        self._worker = Worker(self._load_workspaces_on_thread, row_index_pair, get_states_func)
        self._worker.signals.finished.connect(self.on_finished)
        self._worker.signals.error.connect(self.on_error)

        QThreadPool.globalInstance().start(self._worker)

    def _process_states_on_thread(self, row_index_pair, get_states_func, use_optimizations,
                                  output_mode, plot_results, output_graph, save_can=False):
        for row, index in row_index_pair:

            # TODO update the get_states_func to support one per call
            try:
                states, errors = get_states_func(row_entries=[row])
            except Exception as e:
                self._handle_err(index, e)
                continue

            assert len(states) + len(errors) == 1, \
                "Expected 1 error to return got {0}".format(len(states) + len(errors))

            for error in errors.values():
                self.row_failed_signal.emit(index, error)

            for state in states.values():
                try:
                    out_scale_factors, out_shift_factors = \
                        self.batch_processor([state.all_states], use_optimizations, output_mode, plot_results, output_graph, save_can)
                except Exception as e:
                    self._handle_err(index, e)
                    continue

                if state.all_states.reduction.reduction_mode == ReductionMode.MERGED:
                    out_shift_factors = out_shift_factors[0]
                    out_scale_factors = out_scale_factors[0]
                else:
                    out_shift_factors = []
                    out_scale_factors = []
                self.row_processed_signal.emit(index, out_shift_factors, out_scale_factors)

    def _load_workspaces_on_thread(self, row_index_pair, get_states_func):
        for row, index in row_index_pair:
            try:
                states, errors = get_states_func(row_entries=[row])
            except Exception as e:
                self._handle_err(index, e)
                continue

            for error in errors.values():
                self.row_failed_signal.emit(index, error)

            for state in states.values():
                try:
                    load_workspaces_from_states(state.all_states)
                    self.row_processed_signal.emit(index, [], [])
                except Exception as e:
                    self._handle_err(index, e)
                    continue

    def _handle_err(self, index, e):
        # We manually have to extract out the traceback, since going to a str for Qt signals will strip this
        self._logger.error(''.join(traceback.format_tb(e.__traceback__)))
        self._logger.error(str(e))
        self.row_failed_signal.emit(index, str(e))
