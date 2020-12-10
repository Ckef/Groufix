/**
 * This file is part of groufix.
 * Copyright (c) Stef Velzel. All rights reserved.
 *
 * groufix : graphics engine produced by Stef Velzel.
 * www     : <www.vuzzel.nl>
 */


#ifndef GFX_CORE_WINDOW_H
#define GFX_CORE_WINDOW_H

#include "groufix/core/device.h"
#include "groufix/core/keys.h"
#include "groufix/def.h"
#include <stddef.h>
#include <uchar.h>


/**
 * Window configuration flags.
 */
typedef enum GFXWindowFlags
{
	GFX_WINDOW_BORDERLESS    = 0x0001,
	GFX_WINDOW_FOCUS         = 0x0002,
	GFX_WINDOW_MAXIMIZE      = 0x0004,
	GFX_WINDOW_RESIZABLE     = 0x0008,
	GFX_WINDOW_CAPTURE_MOUSE = 0x0010, // Implies GFX_WINDOW_HIDE_MOUSE.
	GFX_WINDOW_HIDE_MOUSE    = 0x0020,
	GFX_WINDOW_DOUBLE_BUFFER = 0x0040,
	GFX_WINDOW_TRIPLE_BUFFER = 0x0080  // Overrules GFX_WINDOW_DOUBLE_BUFFER.

} GFXWindowFlags;


/**
 * Monitor video mode.
 */
typedef struct GFXVideoMode
{
	size_t       width;
	size_t       height;
	unsigned int refresh;

} GFXVideoMode;


/**
 * Logical monitor definition.
 */
typedef struct GFXMonitor
{
	// User pointer, can be used for any purpose.
	// Defaults to NULL.
	void*       ptr;
	const char* name; // Read-only.

} GFXMonitor;


/**
 * Logical window definition.
 */
typedef struct GFXWindow
{
	// User pointer, can be used for any purpose.
	// Defaults to NULL.
	void* ptr;

	// Event callbacks.
	struct
	{
		void (*close   )(struct GFXWindow*);
		void (*drop    )(struct GFXWindow*, size_t count, const char** paths);
		void (*focus   )(struct GFXWindow*);
		void (*blur    )(struct GFXWindow*);
		void (*maximize)(struct GFXWindow*);
		void (*minimize)(struct GFXWindow*);
		void (*restore )(struct GFXWindow*);
		void (*move    )(struct GFXWindow*, int x, int y);
		void (*resize  )(struct GFXWindow*, size_t width, size_t height);

		// Keyboard events.
		struct
		{
			void (*press  )(struct GFXWindow*, GFXKey, int scan, GFXModifier);
			void (*release)(struct GFXWindow*, GFXKey, int scan, GFXModifier);
			void (*repeat )(struct GFXWindow*, GFXKey, int scan, GFXModifier);
			void (*text   )(struct GFXWindow*, char32_t codepoint);

		} key;

		// Mouse events.
		struct
		{
			void (*enter  )(struct GFXWindow*);
			void (*leave  )(struct GFXWindow*);
			void (*move   )(struct GFXWindow*, double x, double y);
			void (*press  )(struct GFXWindow*, GFXMouseButton, GFXModifier);
			void (*release)(struct GFXWindow*, GFXMouseButton, GFXModifier);
			void (*scroll )(struct GFXWindow*, double x, double y);

		} mouse;

	} events;

} GFXWindow;


/****************************
 * Logical monitor.
 ****************************/

/**
 * Sets the configuration change event callback.
 * The callback takes the monitor in question and a zero or non-zero value,
 * zero if the monitor is disconnected, non-zero if it is connected.
 * @param event NULL to disable the event callback.
 */
GFX_API void gfx_set_monitor_event(void (*event)(GFXMonitor*, int));

/**
 * Retrieves the number of currently connected monitors.
 * @return 0 if no monitors were found.
 */
GFX_API size_t gfx_get_num_monitors(void);

/**
 * Retrieves a currently connected monitor.
 * The primary monitor is always stored at index 0.
 * @param index Must be < gfx_get_num_monitors().
 */
GFX_API GFXMonitor* gfx_get_monitor(size_t index);

/**
 * Retrieves the primary (user's preferred) monitor.
 * This is equivalent to gfx_get_monitor(0).
 */
GFX_API GFXMonitor* gfx_get_primary_monitor(void);

/**
 * Retrieves the number of video modes available for a monitor.
 * @param monitor Cannot be NULL.
 */
GFX_API size_t gfx_monitor_get_num_modes(GFXMonitor* monitor);

/**
 * Retrieves a video mode of a monitor.
 * @param monitor Cannot be NULL.
 * @param index   Must be < gfx_monitor_get_num_modes(monitor).
 */
GFX_API GFXVideoMode gfx_monitor_get_mode(GFXMonitor* monitor, size_t index);

/**
 * Retrieves the current video mode of a monitor.
 * @param monitor Cannot be NULL.
 */
GFX_API GFXVideoMode gfx_monitor_get_current_mode(GFXMonitor* monitor);


/****************************
 * Logical window.
 ****************************/

/**
 * Creates a logical window.
 * @param device  NULL is equivalent to gfx_get_primary_device().
 * @param monitor NULL for windowed mode, fullscreen monitor otherwise.
 * @param mode    Width and height must be > 0.
 * @param title   Cannot be NULL.
 * @return NULL on failure.
 *
 * mode.refresh is ignored if monitor is set to NULL.
 */
GFX_API GFXWindow* gfx_create_window(GFXWindowFlags flags, GFXDevice* device,
                                     GFXMonitor* monitor, GFXVideoMode mode,
                                     const char* title);

/**
 * Destroys a logical window.
 * Must NOT be called from within a window event.
 */
GFX_API void gfx_destroy_window(GFXWindow* window);

/**
 * Retrieves the monitor the window is fullscreened to.
 * @param window Cannot be NULL.
 * @return NULL if the window is not assigned to a monitor.
 */
GFX_API GFXMonitor* gfx_window_get_monitor(GFXWindow* window);

/**
 * Sets the monitor to fullscreen to.
 * @param window  Cannot be NULL.
 * @param monitor NULL for windowed mode, fullscreen monitor otherwise.
 * @param mode    Width and height must be > 0.
 *
 * mode.refresh is ignored if monitor is set to NULL.
 */
GFX_API void gfx_window_set_monitor(GFXWindow* window, GFXMonitor* monitor,
                                    GFXVideoMode mode);

/**
 * Sets a new window title.
 * @param window Cannot be NULL.
 * @param title  Cannot be NULL.
 */
GFX_API void gfx_window_set_title(GFXWindow* window, const char* title);

/**
 * Retrieves whether the close flag is set.
 * This flag is set by either the window manager or gfx_window_set_close.
 * @param window Cannot be NULL.
 */
GFX_API int gfx_window_should_close(GFXWindow* window);

/**
 * Explicitally set the close flag of a window.
 * This is the only way to tell a window to close from within a window event.
 * @param window Cannot be NULL.
 */
GFX_API void gfx_window_set_close(GFXWindow* window, int close);

/**
 * Maximizes the window.
 * @param window Cannot be NULL.
 */
GFX_API void gfx_window_maximize(GFXWindow* window);

/**
 * Minimizes the window.
 * @param window Cannot be NULL.
 */
GFX_API void gfx_window_minimize(GFXWindow* window);

/**
 * Restores the window from maximization or minimization.
 * @param window Cannot be NULL.
 */
GFX_API void gfx_window_restore(GFXWindow* window);


#endif
