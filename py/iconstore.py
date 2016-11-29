"""
This module provides unified access to this projects icons.
"""

from PyQt5.QtGui import *
from PyQt5.QtCore import *

import napclient

# Store icons as type:QIcon
_ICONS = {}

_ICON_NAMES = {
    napclient.Entity: 'entity',
    napclient.Attribute: 'attribute',
    napclient.Component: 'component',
    napclient.Object: 'undefined',
}


def icon(name):
    """ Retrieve an icon by name as such: iconDir/NAME.png,
    where NAME is the provided string. """
    return QIcon('icons/%s.png' % name)

def iconFor(obj):
    """ Based on the type of @obj, return an icon.
    """
    if isinstance(obj, type):
        objType = obj
    else:
        assert(isinstance(obj, napclient.Object))
        global _ICONS
        objType = type(obj)

    if objType in _ICONS:
        return _ICONS[objType]

    if not objType in _ICON_NAMES.keys():
        objType = napclient.Object

    _ICONS[objType] = icon(_ICON_NAMES[objType])
    return _ICONS[objType]

