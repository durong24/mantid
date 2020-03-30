=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:

New
###

- The Advanced Plotting menu is now in Workbench. This enables creating surface and contour plots of three or more workspaces, and choosing which log value to plot against.

Improvements
############

- Tile plots are now reloaded correctly by project recovery.
- Fixed an issue where some scripts were running slower if a  plot was open at the same time.


Bugfixes
########

- Fixed a bug where setting columns to Y error in table workspaces wasn't working. The links between the Y error and Y columns weren't being set up properly
- The scale of the color bars on colorfill plots of ragged workspaces now uses the maximum and minimum values of the data.

:ref:`Release 5.1.0 <v5.1.0>`
