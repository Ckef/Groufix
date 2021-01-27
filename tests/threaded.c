/**
 * This file is part of groufix.
 * Copyright (c) Stef Velzel. All rights reserved.
 *
 * groufix : graphics engine produced by Stef Velzel.
 * www     : <www.vuzzel.nl>
 */

#define TEST_ENABLE_THREADS
#include "test.h"


/****************************
 * The bit that renders.
 */
TEST_DESCRIBE(render_loop, _t)
{
	// Like the other loop, but submit the renderer :)
	// TODO: So a call to gfx_window_should_close is not thread-safe.
	// Actually it sorta is according to GLFW, but not synced.
	while (!gfx_window_should_close(_t->window))
		gfx_renderer_submit(_t->renderer);
}


/****************************
 * Threading test.
 */
TEST_DESCRIBE(threaded, _t)
{
	// Yeah we're gonna have maniest frames here too.
	gfx_window_set_flags(
		_t->window,
		gfx_window_get_flags(_t->window) | GFX_WINDOW_TRIPLE_BUFFER);

	// Create thread to run the render loop.
	TEST_RUN_THREAD(render_loop);

	// Setup an event loop.
	while (!gfx_window_should_close(_t->window))
		gfx_wait_events();

	// Join the render thread.
	TEST_JOIN(render_loop);
}


/****************************
 * Run the threading test.
 */
TEST_MAIN(threaded);
