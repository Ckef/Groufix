/**
 * This file is part of groufix.
 * Copyright (c) Stef Velzel. All rights reserved.
 *
 * groufix : graphics engine produced by Stef Velzel.
 * www     : <www.vuzzel.nl>
 */


#ifndef GFX_CORE_RENDERER_H
#define GFX_CORE_RENDERER_H

#include "groufix/core/device.h"
#include "groufix/core/heap.h" // TODO: Totally temporary!
#include "groufix/core/window.h"
#include "groufix/def.h"


/**
 * Size class of a resource.
 */
typedef enum GFXSizeClass
{
	GFX_SIZE_ABSOLUTE,
	GFX_SIZE_RELATIVE

} GFXSizeClass;


/**
 * Attachment description.
 */
typedef struct GFXAttachment
{
	GFXSizeClass size;
	size_t       ref; // Index of the attachment the size is relative to.

	union {
		size_t width;
		float xScale;
	};

	union {
		size_t height;
		float yScale;
	};

	union {
		size_t depth;
		float zScale;
	};

} GFXAttachment;


/**
 * Renderer definition.
 */
typedef struct GFXRenderer GFXRenderer;


/**
 * Render pass definition.
 */
typedef struct GFXRenderPass GFXRenderPass;


/****************************
 * Renderer handling.
 ****************************/

/**
 * Creates a renderer.
 * @param device NULL is equivalent to gfx_get_primary_device().
 * @return NULL on failure.
 */
GFX_API GFXRenderer* gfx_create_renderer(GFXDevice* device);

/**
 * Destroys a renderer.
 * This will block until rendering using its resources is done!
 */
GFX_API void gfx_destroy_renderer(GFXRenderer* renderer);

/**
 * Describes the properties of an attachment index of the renderer.
 * @param renderer Cannot be NULL.
 * @return Zero on failure.
 *
 * If a window needs to be detached, this will block until rendering is done!
 */
GFX_API int gfx_renderer_attach(GFXRenderer* renderer,
                                size_t index, GFXAttachment attachment);

/**
 * Attaches a window to an attachment index of a renderer.
 * @param renderer Cannot be NULL.
 * @param window   NULL to detach the current window, if any.
 * @return Zero on failure.
 *
 * If a window needs to be detached, this will block until rendering is done!
 * Fails if the window was already attached to a renderer or the window and
 * renderer do not share a compatible device.
 */
GFX_API int gfx_renderer_attach_window(GFXRenderer* renderer,
                                       size_t index, GFXWindow* window);

/**
 * Adds a new (target) render pass to the renderer given a set of dependencies.
 * Each element in deps must be associated with the same renderer.
 * @param renderer Cannot be NULL.
 * @param numDeps  Number of dependencies, 0 for none.
 * @param deps     Passes it depends on, cannot be NULL if numDeps > 0.
 * @return NULL on failure.
 *
 * The renderer shares resources with all passes, it cannot concurrently
 * operate with any pass and passes cannot concurrently operate among themselves.
 * A render pass cannot be removed, nor can its dependencies be changed
 * once it has been added to a renderer.
 */
GFX_API GFXRenderPass* gfx_renderer_add(GFXRenderer* renderer,
                                        size_t numDeps, GFXRenderPass** deps);

/**
 * Retrieves the number of target render passes of a renderer.
 * A target pass is one that no other pass depends on (last in the path).
 * @param renderer Cannot be NULL.
 *
 * This number may change when a new render pass is added.
 */
GFX_API size_t gfx_renderer_get_num_targets(GFXRenderer* renderer);

/**
 * Retrieves a target render pass of a renderer.
 * @param renderer Cannot be NULL.
 * @param target   Target index, must be < gfx_renderer_get_num(renderer).
 *
 * The index of each target may change when a new render pass is added,
 * however their order remains fixed during the lifetime of the renderer.
 */
GFX_API GFXRenderPass* gfx_renderer_get_target(GFXRenderer* renderer,
                                               size_t target);

/**
 * TODO: Totally under construction.
 * Submits all passes of the renderer to the GPU.
 * @param renderer Cannot be NULL.
 * @return Zero if the renderer could not build.
 *
 * Returns non-zero if build was successful. Most errors during submission are
 * considered pseudo-fatal and ignored, processing continues.
 */
GFX_API int gfx_renderer_submit(GFXRenderer* renderer);


/****************************
 * Render pass handling.
 ****************************/

/**
 * Set render pass to read from an attachpent index of the renderer.
 * @param pass  Cannot be NULL.
 * @return Zero on failure.
 */
GFX_API int gfx_render_pass_read(GFXRenderPass* pass, size_t index);

/**
 * TODO: shader location == in add-order?
 * Set render pass to write to an attachment index of the renderer.
 * @see gfx_render_pass_read.
 */
GFX_API int gfx_render_pass_write(GFXRenderPass* pass, size_t index);

/**
 * Retrieves the number of passes a single render pass depends on.
 * @param pass Cannot be NULL.
 */
GFX_API size_t gfx_render_pass_get_num_deps(GFXRenderPass* pass);

/**
 * Retrieves a dependency of a render pass.
 * @param pass Cannot be NULL.
 * @param dep  Dependency index, must be < gfx_render_pass_get_num(pass).
 */
GFX_API GFXRenderPass* gfx_render_pass_get_dep(GFXRenderPass* pass, size_t dep);

/**
 * TODO: Totally temporary!
 * Makes the render pass render the given things.
 */
GFX_API void gfx_render_pass_use(GFXRenderPass* pass, GFXMesh* mesh);


#endif
