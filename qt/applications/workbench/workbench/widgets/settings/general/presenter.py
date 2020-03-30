# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from mantid.kernel import ConfigService
from workbench.config import CONF
from workbench.widgets.settings.general.view import GeneralSettingsView

from qtpy.QtCore import Qt


class GeneralSettings(object):
    """
    Presenter of the General settings section. It handles all changes to options
    within the section, and updates the ConfigService and workbench CONF accordingly.

    If new options are added to the General settings, their events when changed should
    be handled here.
    """

    CRYSTALLOGRAPY_CONV = "Q.convention"
    FONT = "MainWindow/font"
    INSTRUMENT = "default.instrument"
    OPENGL = "MantidOptions.InstrumentView.UseOpenGL"
    SHOW_INVISIBLE_WORKSPACES = "MantidOptions.InvisibleWorkspaces"
    PR_NUMBER_OF_CHECKPOINTS = "projectRecovery.numberOfCheckpoints"
    PR_TIME_BETWEEN_RECOVERY = "projectRecovery.secondsBetween"
    PR_RECOVERY_ENABLED = "projectRecovery.enabled"
    PROMPT_ON_DELETING_WORKSPACE = 'project/prompt_on_deleting_workspace'
    PROMPT_SAVE_EDITOR_MODIFIED = 'project/prompt_save_editor_modified'
    PROMPT_SAVE_ON_CLOSE = 'project/prompt_save_on_close'
    USE_NOTIFICATIONS = 'Notifications.Enabled'
    USER_LAYOUT = "MainWindow/user_layouts"

    def __init__(self, parent, view=None):
        self.view = view if view else GeneralSettingsView(parent, self)
        self.parent = parent
        self.load_current_setting_values()

        self.setup_facilities_group()
        self.setup_checkbox_signals()
        self.setup_general_group()

        self.setup_layout_options()

        self.setup_confirmations()

    def setup_facilities_group(self):
        facilities = ConfigService.getFacilityNames()
        if not facilities:
            return
        self.view.facility.addItems(facilities)

        try:
            default_facility = ConfigService.getFacility().name()
        except RuntimeError:
            default_facility = facilities[0]
        self.view.facility.setCurrentIndex(self.view.facility.findText(default_facility))
        self.action_facility_changed(default_facility)
        self.view.facility.currentTextChanged.connect(self.action_facility_changed)

        try:
            default_instrument = ConfigService.getInstrument().name()
            self.view.instrument.setCurrentIndex(self.view.instrument.findText(default_instrument))
        except RuntimeError:
            default_instrument = self.view.instrument.itemText(0)
        self.action_instrument_changed(default_instrument)
        self.view.instrument.currentTextChanged.connect(self.action_instrument_changed)

    def setup_general_group(self):
        self.view.main_font.clicked.connect(self.action_main_font_button_clicked)

    def action_main_font_button_clicked(self):
        font_dialog = self.view.create_font_dialog(self.parent)
        font_dialog.fontSelected.connect(self.action_font_selected)

    def action_font_selected(self, font):
        CONF.set(self.FONT, font.toString())

    def setup_checkbox_signals(self):
        self.view.show_invisible_workspaces.setChecked(
            "true" == ConfigService.getString(self.SHOW_INVISIBLE_WORKSPACES).lower())

        self.view.show_invisible_workspaces.stateChanged.connect(self.action_show_invisible_workspaces)
        self.view.project_recovery_enabled.stateChanged.connect(self.action_project_recovery_enabled)
        self.view.time_between_recovery.valueChanged.connect(self.action_time_between_recovery)
        self.view.total_number_checkpoints.valueChanged.connect(self.action_total_number_checkpoints)
        self.view.crystallography_convention.stateChanged.connect(self.action_crystallography_convention)
        self.view.use_open_gl.stateChanged.connect(self.action_use_open_gl)

    def action_facility_changed(self, new_facility):
        """
        When the facility is changed, refreshes all available instruments that can be selected in the dropdown.
        :param new_facility: The name of the new facility that was selected
        """
        current_value = ConfigService.getFacility().name()
        if new_facility != current_value:
            ConfigService.setFacility(new_facility)
        # refresh the instrument selection to contain instruments about the selected facility only
        self.view.instrument.clear()
        self.view.instrument.addItems(
            [instr.name() for instr in ConfigService.getFacility(new_facility).instruments()])

    def setup_confirmations(self):
        self.view.prompt_save_on_close.stateChanged.connect(self.action_prompt_save_on_close)
        self.view.prompt_save_editor_modified.stateChanged.connect(self.action_prompt_save_editor_modified)
        self.view.prompt_deleting_workspaces.stateChanged.connect(self.action_prompt_deleting_workspace)
        self.view.use_notifications.stateChanged.connect(self.action_use_notifications_modified)

    def action_prompt_save_on_close(self, state):
        CONF.set(self.PROMPT_SAVE_ON_CLOSE, bool(state))

    def action_prompt_save_editor_modified(self, state):
        CONF.set(self.PROMPT_SAVE_EDITOR_MODIFIED, bool(state))

    def action_prompt_deleting_workspace(self, state):
        CONF.set(self.PROMPT_ON_DELETING_WORKSPACE, bool(state))

    def action_use_notifications_modified(self, state):
        ConfigService.setString(self.USE_NOTIFICATIONS, "On" if bool(state) else "Off")

    def load_current_setting_values(self):
        self.view.prompt_save_on_close.setChecked(bool(CONF.get(self.PROMPT_SAVE_ON_CLOSE)))
        self.view.prompt_save_editor_modified.setChecked(bool(CONF.get(self.PROMPT_SAVE_EDITOR_MODIFIED)))
        self.view.prompt_deleting_workspaces.setChecked(bool(CONF.get(self.PROMPT_ON_DELETING_WORKSPACE)))

        # compare lower-case, because MantidPlot will save it as lower case,
        # but Python will have the bool's first letter capitalised
        pr_enabled = ("true" == ConfigService.getString(self.PR_RECOVERY_ENABLED).lower())
        pr_time_between_recovery = int(ConfigService.getString(self.PR_TIME_BETWEEN_RECOVERY))
        pr_number_checkpoints = int(ConfigService.getString(self.PR_NUMBER_OF_CHECKPOINTS))
        use_notifications_setting = ("on" == ConfigService.getString(self.USE_NOTIFICATIONS).lower())
        crystallography_convention = ("Crystallography" == ConfigService.getString(self.CRYSTALLOGRAPY_CONV))
        use_open_gl = ("on" == ConfigService.getString(self.OPENGL).lower())

        self.view.project_recovery_enabled.setChecked(pr_enabled)
        self.view.time_between_recovery.setValue(pr_time_between_recovery)
        self.view.total_number_checkpoints.setValue(pr_number_checkpoints)
        self.view.use_notifications.setChecked(use_notifications_setting)
        self.view.crystallography_convention.setChecked(crystallography_convention)
        self.view.use_open_gl.setChecked(use_open_gl)

    def action_project_recovery_enabled(self, state):
        ConfigService.setString(self.PR_RECOVERY_ENABLED, str(bool(state)))

    def action_time_between_recovery(self, value):
        ConfigService.setString(self.PR_TIME_BETWEEN_RECOVERY, str(value))

    def action_total_number_checkpoints(self, value):
        ConfigService.setString(self.PR_NUMBER_OF_CHECKPOINTS, str(value))

    def action_crystallography_convention(self, state):
        ConfigService.setString(self.CRYSTALLOGRAPY_CONV, "Crystallography" if state == Qt.Checked else "Inelastic")

    def action_instrument_changed(self, new_instrument):
        current_value = ConfigService.getString(self.INSTRUMENT)
        if new_instrument != current_value:
            ConfigService.setString(self.INSTRUMENT, new_instrument)

    def action_show_invisible_workspaces(self, state):
        ConfigService.setString(self.SHOW_INVISIBLE_WORKSPACES, str(bool(state)))

    def action_use_open_gl(self, state):
        ConfigService.setString(self.OPENGL, "On" if bool(state) else "Off")

    def setup_layout_options(self):
        self.fill_layout_display()
        self.view.save_layout.clicked.connect(self.save_layout)
        self.view.load_layout.clicked.connect(self.load_layout)
        self.view.delete_layout.clicked.connect(self.delete_layout)

    def fill_layout_display(self):
        self.view.layout_display.clear()
        layout_dict = self.get_layout_dict()
        layout_list = sorted(layout_dict.keys())
        for item in layout_list:
            self.view.layout_display.addItem(item)

    def get_layout_dict(self):
        try:
            layout_list = CONF.get(self.USER_LAYOUT)
        except KeyError:
            layout_list = {}
        return layout_list

    def save_layout(self):
        filename = self.view.new_layout_name.text()
        if filename != "":
            layout_dict = self.get_layout_dict()
            layout_dict[filename] = self.parent.saveState()
            CONF.set(self.USER_LAYOUT, layout_dict)
            self.view.new_layout_name.clear()
            self.fill_layout_display()
            self.parent.populate_layout_menu()

    def load_layout(self):
        item = self.view.layout_display.currentItem()
        if hasattr(item, "text"):
            layout = item.text()
            layout_dict = self.get_layout_dict()
            self.parent.restoreState(layout_dict[layout])

    def delete_layout(self):
        item = self.view.layout_display.currentItem()
        if hasattr(item, "text"):
            layout = item.text()
            layout_dict = self.get_layout_dict()
            layout_dict.pop(layout, None)
            CONF.set(self.USER_LAYOUT, layout_dict)
            self.fill_layout_display()
            self.parent.populate_layout_menu()

    def focus_layout_box(self):
        # scroll the settings to the layout box. High yMargin ensures the box is always at the top of the window.
        self.view.scrollArea.ensureWidgetVisible(self.view.new_layout_name, yMargin=1000)
        self.view.new_layout_name.setFocus()
