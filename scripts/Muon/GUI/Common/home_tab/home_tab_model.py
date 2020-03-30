# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class HomeTabModel(object):

    def __init__(self, context=None):
        self._data = context.data_context
        self._context = context

    def is_data_loaded(self):
        return self._data.is_data_loaded()

    def loaded_instrument(self):
        return self._data.instrument

    def show_all_data(self):
        self._context.show_raw_data()
        self._context.show_all_groups()
        self._context.show_all_pairs()
