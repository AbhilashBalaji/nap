import os
import sys

from PyQt5.QtCore import QSettings
from PyQt5.QtWidgets import *

from generic.proxystyle import MyProxyStyle
from generic.qbasewindow import QBaseWindow
from pynap_json.actions import LAST_FILE, OpenFileAction, SaveFileAction, \
    SaveFileAsAction
from pynap_json.napjsonwrap import loadFile
from pynap_json.outlinepanel import OutlinePanel
from pynap_json.inspectorpanel import InspectorPanel


def main():
    # QApplication.setStyle(MyProxyStyle(QApplication.style()))
    app = QApplication(sys.argv)
    win = QBaseWindow()

    objectPanel = OutlinePanel()
    win.addDock('Outline', objectPanel)

    propPanel = InspectorPanel()
    win.addDock('Inspector', propPanel)

    filemenu = QMenu('File', win.menuBar())
    filemenu.addActions([
        OpenFileAction(filemenu, objectPanel.model()),
        SaveFileAction(filemenu, objectPanel.model()),
        SaveFileAsAction(filemenu, objectPanel.model()),
    ])
    win.menuBar().insertMenu(win.windowMenu().menuAction(), filemenu)


    # load last file
    filename = QSettings().value(LAST_FILE)
    if os.path.exists(filename):
        objectPanel.model().setObjects(loadFile(filename))

    objectPanel.selectionChanged.connect(propPanel.setObjects)

    win.show()
    app.exec()


if __name__ == '__main__':
    main()
