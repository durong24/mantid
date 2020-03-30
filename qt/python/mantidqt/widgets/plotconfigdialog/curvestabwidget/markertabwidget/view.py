# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QWidget

from mantidqt.widgets.plotconfigdialog.colorselector import ColorSelector
from mantidqt.utils.qt import load_ui

MARKER_STYLES = ['None', 'point', 'pixel', "circle", "triangle_down", "triangle_up", "triangle_left",
                 "triangle_right", "tri_down", "tri_up", "tri_left", "tri_right", "octagon", "square",
                 "pentagon", "plus (filled)", "star", "hexagon1", "hexagon2", "plus", "x", "x (filled)",
                 "diamond", "thin_diamond", "vline", "hline", "tickleft", "tickright", "tickup",
                 "tickdown", "caretleft", "caretright", "caretup", "caretdown", "caretleft (centered at base)",
                 "caretright (centered at base)", "caretup (centered at base)", "caretdown (centered at base)"]


class MarkerTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(MarkerTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab_marker_tab.ui',
                          baseinstance=self)
        self.face_color_selector_widget = ColorSelector(parent=self)
        self.grid_layout.replaceWidget(self.face_color_dummy_widget,
                                       self.face_color_selector_widget)
        self.edge_color_selector_widget = ColorSelector(parent=self)
        self.grid_layout.replaceWidget(self.edge_color_dummy_widget,
                                       self.edge_color_selector_widget)
        self.ui.marker_style_combo_box.addItems(MARKER_STYLES)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def get_style(self):
        return self.marker_style_combo_box.currentText()

    def set_style(self, style):
        self.marker_style_combo_box.setCurrentText(style)

    def get_size(self):
        return self.marker_size_spin_box.value()

    def set_size(self, size):
        self.marker_size_spin_box.setValue(size)

    def get_face_color(self):
        return self.face_color_selector_widget.get_color()

    def set_face_color(self, color):
        self.face_color_selector_widget.set_color(color)

    def get_edge_color(self):
        return self.edge_color_selector_widget.get_color()

    def set_edge_color(self, color):
        self.edge_color_selector_widget.set_color(color)

    def update_fields(self, curve_props):
        self.set_style(curve_props.marker)
        self.set_size(curve_props.markersize)
        self.set_face_color(curve_props.markerfacecolor)
        self.set_edge_color(curve_props.markeredgecolor)

    def set_apply_to_all_enabled(self, enable):
        self.apply_to_all_button.setEnabled(enable)
