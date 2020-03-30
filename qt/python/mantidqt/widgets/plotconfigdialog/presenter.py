# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantidqt.widgets.plotconfigdialog import curve_in_figure, image_in_figure, legend_in_figure
from mantidqt.widgets.plotconfigdialog.view import PlotConfigDialogView
from mantidqt.widgets.plotconfigdialog.axestabwidget.presenter import AxesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter import CurvesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.imagestabwidget.presenter import ImagesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.legendtabwidget.presenter import LegendTabWidgetPresenter


class PlotConfigDialogPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if view:
            self.view = view
        else:
            self.view = PlotConfigDialogView(parent)
        self.view.show()

        self.tab_widget_presenters = [None, None, None, None]
        self.tab_widget_views = [None, None, None, None]
        legend_tab = None
        # Axes tab
        if len(self.fig.get_axes()) > 0:
            axes_tab = AxesTabWidgetPresenter(self.fig, parent=self.view)
            self.tab_widget_presenters[1] = axes_tab
            self.tab_widget_views[0] = (axes_tab.view, "Axes")
        # Legend tab
        if legend_in_figure(self.fig):
            legend_tab = LegendTabWidgetPresenter(self.fig, parent=self.view)
            self.tab_widget_presenters[0] = legend_tab
            self.tab_widget_views[3] = (legend_tab.view, "Legend")
        # Curves tab
        if curve_in_figure(self.fig):
            curves_tab = CurvesTabWidgetPresenter(self.fig, parent=self.view, legend_tab=legend_tab)
            self.tab_widget_presenters[2] = curves_tab
            self.tab_widget_views[1] = (curves_tab.view, "Curves")
        # Images tab
        if image_in_figure(self.fig):
            images_tab = ImagesTabWidgetPresenter(self.fig, parent=self.view)
            self.tab_widget_presenters[3] = images_tab
            self.tab_widget_views[2] = (images_tab.view, "Images")

        self._add_tab_widget_views()

        # Signals
        self.view.ok_button.clicked.connect(self.apply_properties_and_exit)
        self.view.apply_button.clicked.connect(self.apply_properties)
        self.view.cancel_button.clicked.connect(self.exit)

    def _add_tab_widget_views(self):
        for tab_view in self.tab_widget_views:
            if tab_view:
                self.view.add_tab_widget(tab_view)

    def apply_properties(self):
        for tab in reversed(self.tab_widget_presenters):
            if tab and tab.view:
                tab.apply_properties()
        self.fig.canvas.draw()

    def apply_properties_and_exit(self):
        self.apply_properties()
        self.exit()

    def exit(self):
        self.view.close()
