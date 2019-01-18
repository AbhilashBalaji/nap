#ifdef NAP_ENABLE_PYTHON
	#include <rtti/pythonmodule.h>
#endif
#include "windowevent.h"

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowEvent)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ParameterizedWindowEvent)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowShownEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowHiddenEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowMovedEvent)
	RTTI_CONSTRUCTOR(int, int, int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowResizedEvent)
	RTTI_CONSTRUCTOR(int, int, int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowMinimizedEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowMaximizedEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowRestoredEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowEnterEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowLeaveEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowFocusGainedEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowFocusLostEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowCloseEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowExposedEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WindowTakeFocusEvent)
	RTTI_CONSTRUCTOR(int)
RTTI_END_CLASS