/**
 * This file is part of groufix.
 * Copyright (c) Stef Velzel. All rights reserved.
 *
 * groufix : graphics engine produced by Stef Velzel.
 * www     : <www.vuzzel.nl>
 */

#include "groufix/core.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


#define _GFX_GET_DEVICE_PROC_ADDR(pName) \
	context->vk.pName = (PFN_vk##pName)_groufix.vk.GetDeviceProcAddr( \
		context->vk.device, "vk"#pName); \
	if (context->vk.pName == NULL) { \
		gfx_log_error("Could not load vk"#pName"."); \
		goto clean; \
	}

#define _GFX_GET_DEVICE_TYPE(vType) \
	((vType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? \
		GFX_DEVICE_DISCRETE_GPU : \
	(vType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) ? \
		GFX_DEVICE_VIRTUAL_GPU : \
	(vType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) ? \
		GFX_DEVICE_INTEGRATED_GPU : \
	(vType == VK_PHYSICAL_DEVICE_TYPE_CPU) ? \
		GFX_DEVICE_CPU : \
		GFX_DEVICE_UNKNOWN)

// Gets the complete set of queue flags (adding optional left out bits).
#define _GFX_QUEUE_FLAGS_ALL(vFlags) \
	((vFlags & VK_QUEUE_GRAPHICS_BIT || vFlags & VK_QUEUE_COMPUTE_BIT) ? \
		vFlags | VK_QUEUE_TRANSFER_BIT : vFlags)

// Counts the number of (relevant) set bits in a set of queue flags.
#define _GFX_QUEUE_FLAGS_COUNT(vFlags) \
	((vFlags & VK_QUEUE_GRAPHICS_BIT ? 1 : 0) + \
	(vFlags & VK_QUEUE_COMPUTE_BIT ? 1 : 0) + \
	(vFlags & VK_QUEUE_TRANSFER_BIT ? 1 : 0))


/****************************
 * Array of Vulkan queue priority values in [0,1].
 * TODO: For now just a single queue, maybe we want more with varying values?
 */
static const float _gfx_vk_queue_priorities[] = { 1.0f };


/****************************
 * Fills a VkPhysicalDeviceFeatures struct with features to enable,
 * in other words; it disables feature we don't want.
 * @param device Cannot be NULL.
 * @param pdf    Output data, cannot be NULL.
 */
static void _gfx_get_device_features(_GFXDevice* device,
                                     VkPhysicalDeviceFeatures* pdf)
{
	assert(device != NULL);
	assert(pdf != NULL);

	_groufix.vk.GetPhysicalDeviceFeatures(device->vk.device, pdf);

	// For features we do want, warn if not present.
	if (pdf->geometryShader == VK_FALSE) gfx_log_warn(
		"Physical device does not support geometry shaders: %s.",
		device->name);

	if (pdf->tessellationShader == VK_FALSE) gfx_log_warn(
		"Physical device does not support tessellation shaders: %s.",
		device->name);

	pdf->robustBufferAccess                      = VK_FALSE;
	pdf->fullDrawIndexUint32                     = VK_FALSE;
	pdf->imageCubeArray                          = VK_FALSE;
	pdf->independentBlend                        = VK_FALSE;
	pdf->sampleRateShading                       = VK_FALSE;
	pdf->dualSrcBlend                            = VK_FALSE;
	pdf->logicOp                                 = VK_FALSE;
	pdf->multiDrawIndirect                       = VK_FALSE;
	pdf->drawIndirectFirstInstance               = VK_FALSE;
	pdf->depthClamp                              = VK_FALSE;
	pdf->depthBiasClamp                          = VK_FALSE;
	pdf->fillModeNonSolid                        = VK_FALSE;
	pdf->depthBounds                             = VK_FALSE;
	pdf->wideLines                               = VK_FALSE;
	pdf->largePoints                             = VK_FALSE;
	pdf->alphaToOne                              = VK_FALSE;
	pdf->multiViewport                           = VK_FALSE;
	pdf->samplerAnisotropy                       = VK_FALSE;
	pdf->textureCompressionETC2                  = VK_FALSE;
	pdf->textureCompressionASTC_LDR              = VK_FALSE;
	pdf->textureCompressionBC                    = VK_FALSE;
	pdf->occlusionQueryPrecise                   = VK_FALSE;
	pdf->pipelineStatisticsQuery                 = VK_FALSE;
	pdf->vertexPipelineStoresAndAtomics          = VK_FALSE;
	pdf->fragmentStoresAndAtomics                = VK_FALSE;
	pdf->shaderTessellationAndGeometryPointSize  = VK_FALSE;
	pdf->shaderImageGatherExtended               = VK_FALSE;
	pdf->shaderStorageImageExtendedFormats       = VK_FALSE;
	pdf->shaderStorageImageMultisample           = VK_FALSE;
	pdf->shaderStorageImageReadWithoutFormat     = VK_FALSE;
	pdf->shaderStorageImageWriteWithoutFormat    = VK_FALSE;
	pdf->shaderUniformBufferArrayDynamicIndexing = VK_FALSE;
	pdf->shaderSampledImageArrayDynamicIndexing  = VK_FALSE;
	pdf->shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
	pdf->shaderStorageImageArrayDynamicIndexing  = VK_FALSE;
	pdf->shaderClipDistance                      = VK_FALSE;
	pdf->shaderCullDistance                      = VK_FALSE;
	pdf->shaderFloat64                           = VK_FALSE;
	pdf->shaderInt64                             = VK_FALSE;
	pdf->shaderInt16                             = VK_FALSE;
	pdf->shaderResourceResidency                 = VK_FALSE;
	pdf->shaderResourceMinLod                    = VK_FALSE;
	pdf->sparseBinding                           = VK_FALSE;
	pdf->sparseResidencyBuffer                   = VK_FALSE;
	pdf->sparseResidencyImage2D                  = VK_FALSE;
	pdf->sparseResidencyImage3D                  = VK_FALSE;
	pdf->sparseResidency2Samples                 = VK_FALSE;
	pdf->sparseResidency4Samples                 = VK_FALSE;
	pdf->sparseResidency8Samples                 = VK_FALSE;
	pdf->sparseResidency16Samples                = VK_FALSE;
	pdf->sparseResidencyAliased                  = VK_FALSE;
	pdf->variableMultisampleRate                 = VK_FALSE;
	pdf->inheritedQueries                        = VK_FALSE;
}

/****************************
 * Retrieves the device group a device is part of.
 * @param context Populates its numDevices and devices members.
 * @param device  Cannot be NULL.
 * @param index   Output device index into the group, cannot be NULL.
 * @return Zero on failure.
 */
static int _gfx_get_device_group(_GFXContext* context, _GFXDevice* device,
                                 size_t* index)
{
	assert(context != NULL);
	assert(device != NULL);
	assert(index != NULL);

	// Enumerate all device groups.
	uint32_t cnt;
	_GFX_VK_CHECK(_groufix.vk.EnumeratePhysicalDeviceGroups(
		_groufix.vk.instance, &cnt, NULL), return 0);

	if (cnt == 0) return 0;

	VkPhysicalDeviceGroupProperties groups[cnt];

	_GFX_VK_CHECK(_groufix.vk.EnumeratePhysicalDeviceGroups(
		_groufix.vk.instance, &cnt, groups), return 0);

	// Loop over all groups and see if one contains the device.
	// We take the first device group we find it in, this assumes a device is
	// never seen in multiple groups, which should be reasonable...
	size_t g, i;

	for (g = 0; g < cnt; ++g)
	{
		for (i = 0; i < groups[g].physicalDeviceCount; ++i)
			if (groups[g].physicalDevices[i] == device->vk.device)
				break;

		if (i < groups[g].physicalDeviceCount)
			break;
	}

	if (g >= cnt)
	{
		// Probably want to know when a device is somehow invalid..
		gfx_log_error(
			"Physical device could not be found in any device group: %s.",
			device->name);

		return 0;
	}

	*index = i;
	context->numDevices = groups[g].physicalDeviceCount;

	memcpy(
		context->devices,
		groups[g].physicalDevices,
		sizeof(VkPhysicalDevice) * groups[g].physicalDeviceCount);

	return 1;
}

/****************************
 * Finds the optimal (least flags) queue family from count properties
 * that includes the required flags and presentation support.
 * @param props Cannot be NULL if count > 0.
 * @return Index into props, UINT32_MAX if none found.
 */
static uint32_t _gfx_find_queue_family(_GFXDevice* device, uint32_t count,
                                       VkQueueFamilyProperties* props,
                                       VkQueueFlags flags, int present)
{
	assert(device != NULL);
	assert(count == 0 || props != NULL);

	// Since we don't know anything about the order of the queues,
	// we loop over all the things and keep track of the best fit.
	uint32_t found = UINT32_MAX;
	VkQueueFlags foundFlags;

	for (uint32_t i = 0; i < count; ++i)
	{
		VkQueueFlags iFlags =
			_GFX_QUEUE_FLAGS_ALL(props[i].queueFlags);

		// If it does not include all required flags OR
		// it needs presentation support but it doesn't have it,
		// skip it.
		// Note we do not check for presentation to a specific surface yet.
		// (make sure to short circuit the presentation call tho..)
		if (
			(iFlags & flags) != flags ||
			(present && !glfwGetPhysicalDevicePresentationSupport(
				_groufix.vk.instance, device->vk.device, i)))
		{
			continue;
		}

		// Evaluate if it's better, i.e. check which has less flags.
		if (
			found == UINT32_MAX ||
			_GFX_QUEUE_FLAGS_COUNT(iFlags) < _GFX_QUEUE_FLAGS_COUNT(foundFlags))
		{
			found = i;
			foundFlags = iFlags;
		}
	}

	return found;
}

/****************************
 * Allocates a new queue set.
 * @param context    Appends created set to its sets member.
 * @param count      Number of queues/mutexes to create, must be > 0.
 * @param createInfo Queue create info output, cannot be NULL.
 * @return Non-zero on success.
 */
static int _gfx_alloc_queue_set(_GFXContext* context,
                                uint32_t family, int present, size_t count,
                                VkDeviceQueueCreateInfo* createInfo,
                                VkQueueFlags flags)
{
	assert(context != NULL);
	assert(count > 0);
	assert(createInfo != NULL);

	// Allocate a new queue set.
	_GFXQueueSet* set = malloc(sizeof(_GFXQueueSet) + sizeof(_GFXMutex) * count);
	if (set == NULL) return 0;

	set->family  = family;
	set->flags   = flags;
	set->present = present;

	// Keep inserting a mutex for each queue and stop as soon as we fail.
	for (set->count = 0; set->count < count; ++set->count)
		if (!_gfx_mutex_init(&set->locks[set->count]))
		{
			while (set->count > 0) _gfx_mutex_clear(&set->locks[--set->count]);
			free(set);

			return 0;
		}

	// Insert into set list of the context.
	gfx_list_insert_after(&context->sets, &set->list, NULL);

	// Fill create info.
	// TODO: There is only 1 value for priorities for now.
	*createInfo = (VkDeviceQueueCreateInfo){
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,

		.pNext            = NULL,
		.flags            = 0,
		.queueFamilyIndex = family,
		.queueCount       = (uint32_t)count,
		.pQueuePriorities = _gfx_vk_queue_priorities
	};

	return 1;
}

/****************************
 * Creates an array of VkDeviceQueueCreateInfo structures and fills the
 * _GFXQueueSet list of context, on failure, no list elements are freed!
 * @param createInfos Output create info, must call free() on success.
 * @return Number of created queue sets.
 *
 * Output describe the queue families desired by the groufix implementation.
 */
static size_t _gfx_create_queue_sets(_GFXContext* context, _GFXDevice* device,
                                     VkDeviceQueueCreateInfo** createInfos)
{
	assert(context != NULL);
	assert(device != NULL);
	assert(createInfos != NULL);
	assert(*createInfos == NULL);

	// So get all queue families.
	uint32_t count;
	_groufix.vk.GetPhysicalDeviceQueueFamilyProperties(
		device->vk.device, &count, NULL);

	VkQueueFamilyProperties props[count];
	_groufix.vk.GetPhysicalDeviceQueueFamilyProperties(
		device->vk.device, &count, props);

	// We need a few different queues for different operations.
	// 1) A general graphics family:
	// We use the most optimal family with VK_QUEUE_GRAPHICS_BIT set.
	// 2) A family that supports presentation to surface:
	// Preferably the graphics queue, otherwise another one.
	// 3) A transfer family:
	// We use the most optimal family with VK_QUEUE_TRANSFER_BIT set.
	// TODO: 3) A compute-only family for use when others are stalling.

	// UINT32_MAX means no such queue is found.
	uint32_t graphics = UINT32_MAX;
	uint32_t present = UINT32_MAX;
	uint32_t transfer = UINT32_MAX;

	// Start with finding a graphics family, hopefully with presentation.
	// Oh and find a (hopefully better) transfer queue.
	graphics = _gfx_find_queue_family(
		device, count, props, VK_QUEUE_GRAPHICS_BIT, 1);
	transfer = _gfx_find_queue_family(
		device, count, props, VK_QUEUE_TRANSFER_BIT, 0);

	if (graphics != UINT32_MAX)
		present = graphics;
	else
	{
		// If no graphics family with presentation found, find separate queues.
		graphics = _gfx_find_queue_family(
			device, count, props, VK_QUEUE_GRAPHICS_BIT, 0);
		present = _gfx_find_queue_family(
			device, count, props, 0, 1);
	}

	// Now check if we found all queues (and log for all).
	if (graphics == UINT32_MAX) gfx_log_error(
		"Physical device lacks a queue family with VK_QUEUE_GRAPHICS_BIT set: %s.",
		device->name);

	if (present == UINT32_MAX) gfx_log_error(
		"Physical device lacks a queue family with presentation support: %s.",
		device->name);

	if (transfer == UINT32_MAX) gfx_log_error(
		"Physical device lacks a queue family with VK_QUEUE_TRANSFER_BIT set: %s.",
		device->name);

	if (graphics == UINT32_MAX || present == UINT32_MAX || transfer == UINT32_MAX)
		return 0;

	// If transfer queue is not a lone transfer queue, don't use it.
	if (_GFX_QUEUE_FLAGS_COUNT(
		_GFX_QUEUE_FLAGS_ALL(props[transfer].queueFlags)) > 1)
	{
		// Instead use the graphics queue like a pleb :(
		transfer = graphics;
	}

	// Ok so we found all queues, we should now allocate the queue sets and
	// info structures for Vulkan.
	// Allocate the maximum number of infostructures.
	*createInfos = malloc(sizeof(VkDeviceQueueCreateInfo) * 3); // 3!
	if (*createInfos == NULL)
		return 0;

	// Allocate queue sets and count how many.
	size_t sets = 0;

	// TODO: Maybe more queues if the present/transfer queue are the same?
	// Allocate main (graphics) queue.
	int success = _gfx_alloc_queue_set(context,
		graphics, present == graphics, 1, (*createInfos) + (sets++),
		VK_QUEUE_GRAPHICS_BIT |
		(transfer == graphics ? VK_QUEUE_TRANSFER_BIT : 0));

	// Allocate novel present queue if necessary.
	if (present != graphics)
		success = success && _gfx_alloc_queue_set(context,
			present, 1, 1, (*createInfos) + (sets++),
			(transfer == present ? VK_QUEUE_TRANSFER_BIT : 0));

	// Allocate a novel transfer queue if necessary.
	if (transfer != graphics && transfer != present)
		success = success && _gfx_alloc_queue_set(context,
			transfer, 0, 1, (*createInfos) + (sets++),
			VK_QUEUE_TRANSFER_BIT);

	// Check if we succeeded..
	if (!success)
	{
		free(*createInfos);
		*createInfos = NULL;
		return 0;
	}

	return sets;
}

/****************************
 * Destroys a context and all its resources.
 * @param context Cannot be NULL.
 */
static void _gfx_destroy_context(_GFXContext* context)
{
	assert(context != NULL);

	// Erase itself from the context list.
	gfx_list_erase(&_groufix.contexts, &context->list);

	// Loop over all its queue sets and free their resources.
	while (context->sets.head != NULL)
	{
		_GFXQueueSet* set = (_GFXQueueSet*)context->sets.head;
		gfx_list_erase(&context->sets, context->sets.head);

		for (size_t q = 0; q < set->count; ++q)
			_gfx_mutex_clear(&set->locks[q]);

		free(set);
	}

	// We wait for all queues of the device to complete, then we can destroy.
	// We check if the functions were loaded properly,
	// they may not be if something failed during context creation.
	if (context->vk.DeviceWaitIdle != NULL)
		context->vk.DeviceWaitIdle(context->vk.device);
	if (context->vk.DestroyDevice != NULL)
		context->vk.DestroyDevice(context->vk.device, NULL);

	gfx_list_clear(&context->sets);
	free(context);
}

/****************************
 * Creates an appropriate context (Vulkan device + fp's) suited for a device.
 * device->context must be NULL, no prior context can be assigned.
 * @param device Cannot be NULL.
 *
 * Not thread-safe for the same device, it modifies.
 * device->context will remain NULL on failure, on success it will be set to
 * the newly created context (context->index will be set also).
 */
static void _gfx_create_context(_GFXDevice* device)
{
	assert(_groufix.vk.instance != NULL);
	assert(device != NULL);
	assert(device->context == NULL);

	// First of all, check Vulkan version.
	if (device->api < _GFX_VK_VERSION)
	{
		gfx_log_error("Physical device does not support Vulkan version %u.%u.%u: %s.",
			(unsigned int)VK_VERSION_MAJOR(_GFX_VK_VERSION),
			(unsigned int)VK_VERSION_MINOR(_GFX_VK_VERSION),
			(unsigned int)VK_VERSION_PATCH(_GFX_VK_VERSION),
			device->name);

		goto error;
	}

	// Define this preemptively, this will be explicitly freed.
	VkDeviceQueueCreateInfo* createInfos = NULL;

	// Allocate a new context, we are allocating an array of physical devices
	// at the end, just allocate the maximum number, who cares..
	// These are used to check if a future device can use this context.
	_GFXContext* context = malloc(
		sizeof(_GFXContext) +
		sizeof(VkPhysicalDevice) * VK_MAX_DEVICE_GROUP_SIZE);

	if (context == NULL)
		goto error;

	// Set these to NULL so we don't accidentally call garbage on cleanup.
	context->vk.DestroyDevice = NULL;
	context->vk.DeviceWaitIdle = NULL;

	gfx_list_insert_after(&_groufix.contexts, &context->list, NULL);
	gfx_list_init(&context->sets);

	// Now find the device group which this device is part of.
	// This fills numDevices and devices of context!
	size_t index;
	if (!_gfx_get_device_group(context, device, &index))
		goto clean;

	// Call the thing that allocates the desired queues (i.e. fills sets of context!)
	// and gets us the creation info to pass to Vulkan.
	// When a a future device also uses this context, it is assumed it has
	// equivalent queue family properties.
	// If there are any device groups such that this is the case, you
	// probably have equivalent GPUs in an SLI/CrossFire setup anyway...
	size_t sets;
	if (!(sets = _gfx_create_queue_sets(context, device, &createInfos)))
		goto clean;

	// Get desired device features.
	// Similarly to the families, we assume that any device that uses the same
	// context has equivalent features.
	VkPhysicalDeviceFeatures pdf;
	_gfx_get_device_features(device, &pdf);

	// Finally go create the logical Vulkan device.
	// Enable VK_KHR_swapchain so we can interact with surfaces from GLFW.
	// TODO: Enable VK_EXT_memory_budget?
	// Enable VK_LAYER_KHRONOS_validation if debug,
	// this is deprecated by now, but for older Vulkan versions.
	const char* extensions[] = { "VK_KHR_swapchain" };
#if !defined (NDEBUG)
	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
#endif

	VkDeviceGroupDeviceCreateInfo dgdci = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,

		.pNext               = NULL,
		.physicalDeviceCount = (uint32_t)context->numDevices,
		.pPhysicalDevices    = context->devices
	};

	VkDeviceCreateInfo dci = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,

		.pNext                   = &dgdci,
		.flags                   = 0,
		.queueCreateInfoCount    = (uint32_t)sets,
		.pQueueCreateInfos       = createInfos,
#if defined (NDEBUG)
		.enabledLayerCount       = 0,
		.ppEnabledLayerNames     = NULL,
#else
		.enabledLayerCount       = sizeof(layers)/sizeof(char*),
		.ppEnabledLayerNames     = layers,
#endif
		.enabledExtensionCount   = sizeof(extensions)/sizeof(char*),
		.ppEnabledExtensionNames = extensions,
		.pEnabledFeatures        = &pdf
	};

	_GFX_VK_CHECK(_groufix.vk.CreateDevice(
		device->vk.device, &dci, NULL, &context->vk.device), goto clean);

#if !defined (NDEBUG)
	// This is like a moment to celebrate, right?
	// We count the number of actual queues here.
	size_t queueCount = 0;
	for (GFXListNode* k = context->sets.head; k != NULL; k = k->next)
		queueCount += ((_GFXQueueSet*)k)->count;

	gfx_log_debug(
		"Logical Vulkan device of version %u.%u.%u created:\n"
		"    Contains at least: %s.\n"
		"    #physical devices: %u.\n"
		"    #queue sets: %u.\n"
		"    #queues (total): %u.\n",
		(unsigned int)VK_VERSION_MAJOR(device->api),
		(unsigned int)VK_VERSION_MINOR(device->api),
		(unsigned int)VK_VERSION_PATCH(device->api),
		device->name,
		(unsigned int)context->numDevices,
		(unsigned int)sets,
		(unsigned int)queueCount);
#endif

	// Now load all device level Vulkan functions.
	// Load vkDestroyDevice first so we can clean properly.
	_GFX_GET_DEVICE_PROC_ADDR(DestroyDevice);
	_GFX_GET_DEVICE_PROC_ADDR(AcquireNextImageKHR);
	_GFX_GET_DEVICE_PROC_ADDR(AllocateCommandBuffers);
	_GFX_GET_DEVICE_PROC_ADDR(AllocateMemory);
	_GFX_GET_DEVICE_PROC_ADDR(BindBufferMemory);
	_GFX_GET_DEVICE_PROC_ADDR(BeginCommandBuffer);
	_GFX_GET_DEVICE_PROC_ADDR(CmdBeginRenderPass);
	_GFX_GET_DEVICE_PROC_ADDR(CmdBindIndexBuffer);
	_GFX_GET_DEVICE_PROC_ADDR(CmdBindPipeline);
	_GFX_GET_DEVICE_PROC_ADDR(CmdBindVertexBuffers);
	_GFX_GET_DEVICE_PROC_ADDR(CmdDraw);
	_GFX_GET_DEVICE_PROC_ADDR(CmdDrawIndexed);
	_GFX_GET_DEVICE_PROC_ADDR(CmdEndRenderPass);
	_GFX_GET_DEVICE_PROC_ADDR(CreateBuffer);
	_GFX_GET_DEVICE_PROC_ADDR(CreateCommandPool);
	_GFX_GET_DEVICE_PROC_ADDR(CreateFence);
	_GFX_GET_DEVICE_PROC_ADDR(CreateFramebuffer);
	_GFX_GET_DEVICE_PROC_ADDR(CreateGraphicsPipelines);
	_GFX_GET_DEVICE_PROC_ADDR(CreateImageView);
	_GFX_GET_DEVICE_PROC_ADDR(CreatePipelineLayout);
	_GFX_GET_DEVICE_PROC_ADDR(CreateRenderPass);
	_GFX_GET_DEVICE_PROC_ADDR(CreateSemaphore);
	_GFX_GET_DEVICE_PROC_ADDR(CreateShaderModule);
	_GFX_GET_DEVICE_PROC_ADDR(CreateSwapchainKHR);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyBuffer);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyCommandPool);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyFence);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyFramebuffer);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyImageView);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyPipeline);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyPipelineLayout);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyRenderPass);
	_GFX_GET_DEVICE_PROC_ADDR(DestroySemaphore);
	_GFX_GET_DEVICE_PROC_ADDR(DestroyShaderModule);
	_GFX_GET_DEVICE_PROC_ADDR(DestroySwapchainKHR);
	_GFX_GET_DEVICE_PROC_ADDR(DeviceWaitIdle);
	_GFX_GET_DEVICE_PROC_ADDR(EndCommandBuffer);
	_GFX_GET_DEVICE_PROC_ADDR(FreeCommandBuffers);
	_GFX_GET_DEVICE_PROC_ADDR(FreeMemory);
	_GFX_GET_DEVICE_PROC_ADDR(GetBufferMemoryRequirements);
	_GFX_GET_DEVICE_PROC_ADDR(GetDeviceQueue);
	_GFX_GET_DEVICE_PROC_ADDR(GetSwapchainImagesKHR);
	_GFX_GET_DEVICE_PROC_ADDR(MapMemory);
	_GFX_GET_DEVICE_PROC_ADDR(QueuePresentKHR);
	_GFX_GET_DEVICE_PROC_ADDR(QueueSubmit);
	_GFX_GET_DEVICE_PROC_ADDR(QueueWaitIdle);
	_GFX_GET_DEVICE_PROC_ADDR(ResetCommandPool);
	_GFX_GET_DEVICE_PROC_ADDR(ResetFences);
	_GFX_GET_DEVICE_PROC_ADDR(UnmapMemory);
	_GFX_GET_DEVICE_PROC_ADDR(WaitForFences);

	// Set device's reference to this context.
	device->index = index;
	device->context = context;

	free(createInfos);

	return;


	// Cleanup on failure.
clean:
	_gfx_destroy_context(context);
	free(createInfos);
error:
	gfx_log_error(
		"Could not create or initialize a logical Vulkan device for physical "
		"device group containing at least: %s.",
		device->name);
}

/****************************/
int _gfx_devices_init(void)
{
	assert(_groufix.vk.instance != NULL);
	assert(_groufix.devices.size == 0);

	// Reserve and create groufix devices.
	// The number or order of devices never changes after initialization,
	// nor is there a user pointer for callbacks, as there are no callbacks.
	// This means we do not have to dynamically allocate the devices.
	uint32_t count;
	_GFX_VK_CHECK(_groufix.vk.EnumeratePhysicalDevices(
		_groufix.vk.instance, &count, NULL), goto terminate);

	if (count == 0)
		goto terminate;

	// We use a scope here so the goto above is allowed.
	{
		// Enumerate all devices.
		VkPhysicalDevice devices[count];

		_GFX_VK_CHECK(_groufix.vk.EnumeratePhysicalDevices(
			_groufix.vk.instance, &count, devices), goto terminate);

		// Fill the array of groufix devices.
		// While doing so, keep track of the primary device,
		// this to make sure the primary device is at index 0.
		if (!gfx_vec_reserve(&_groufix.devices, (size_t)count))
			goto terminate;

		GFXDeviceType type = GFX_DEVICE_UNKNOWN;
		uint32_t ver = 0;

		// We keep moving around all the devices to sort the primary one to
		// the front, so we leave its mutex and name pointer blank.
		for (uint32_t i = 0; i < count; ++i)
		{
			// Get some Vulkan properties and define a new device.
			VkPhysicalDeviceProperties pdp;
			_groufix.vk.GetPhysicalDeviceProperties(devices[i], &pdp);

			_GFXDevice dev = {
				.base = {
					.type = _GFX_GET_DEVICE_TYPE(pdp.deviceType),
					.name = NULL
				},
				.api     = pdp.apiVersion,
				.index   = 0,
				.context = NULL,
				.vk      = { .device = devices[i] }
			};

			memcpy(
				dev.name, pdp.deviceName,
				VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);

			// Check if the new device is a better pick as primary.
			// If the type of device is superior, pick it as primary.
			// If the type is equal, pick the greater Vulkan version.
			// TODO: Select primary based on physical device features.
			int isPrim = (i == 0) ||
				dev.base.type < type ||
				(dev.base.type == type && pdp.apiVersion > ver);

			if (!isPrim)
				gfx_vec_push(&_groufix.devices, 1, &dev);
			else
			{
				// If new primary, insert it at index 0.
				gfx_vec_insert(&_groufix.devices, 1, &dev, 0);
				type = dev.base.type;
				ver = pdp.apiVersion;
			}
		}

		// Now loop over 'm again to init its mutex and
		// point the public name pointer to the right smth.
		// Because the number of devices never changes, the vector never
		// gets reallocated, thus we store & init these mutexes here.
		for (uint32_t i = 0; i < count; ++i)
		{
			_GFXDevice* dev = gfx_vec_at(&_groufix.devices, i);
			dev->base.name = dev->name;

			if (!_gfx_mutex_init(&dev->lock))
			{
				// If it could not init, clear all previous devices.
				while (i > 0)
				{
					dev = gfx_vec_at(&_groufix.devices, --i);
					_gfx_mutex_clear(&dev->lock);
				}

				gfx_vec_clear(&_groufix.devices);
				goto terminate;
			}
		}

		return 1;
	}


	// Cleanup on failure.
terminate:
	gfx_log_error("Could not find or initialize physical devices.");
	_gfx_devices_terminate();

	return 0;
}

/****************************/
void _gfx_devices_terminate(void)
{
	// Destroy all Vulkan contexts.
	while (_groufix.contexts.head != NULL)
		_gfx_destroy_context((_GFXContext*)_groufix.contexts.head);

	// And free all groufix devices, this only entails clearing its mutex.
	// Devices are allocated in-place so no need to free anything else.
	for (size_t i = 0; i < _groufix.devices.size; ++i)
		_gfx_mutex_clear(&((_GFXDevice*)gfx_vec_at(&_groufix.devices, i))->lock);

	// Regular cleanup.
	gfx_vec_clear(&_groufix.devices);
	gfx_list_clear(&_groufix.contexts);
}

/****************************/
_GFXContext* _gfx_device_init_context(_GFXDevice* device)
{
	assert(device != NULL);

	// Lock the device's lock to sync access to the device's context.
	// Once this call returns successfully the context will not be modified anymore,
	// which means after this call, we can just read device->context directly.
	_gfx_mutex_lock(&device->lock);

	if (device->context == NULL)
	{
		// We only use the context lock here to sync the context array.
		// Other uses happen during initialization or termination,
		// any other operation must happen inbetween those two
		// function calls anyway so no need to lock in them.
		_gfx_mutex_lock(&_groufix.contextLock);

		// No context, go search for a compatible one.
		for (
			_GFXContext* context = (_GFXContext*)_groufix.contexts.head;
			context != NULL;
			context = (_GFXContext*)context->list.next)
		{
			for (size_t j = 0; j < context->numDevices; ++j)
				if (context->devices[j] == device->vk.device)
				{
					device->index = j;
					device->context = context;

					goto unlock;
				}
		}

		// If none found, create a new one.
		// It returns if it was successful, but just ignore it...
		_gfx_create_context(device);

	unlock:
		_gfx_mutex_unlock(&_groufix.contextLock);
	}

	// Read the result before unlock just in case it failed,
	// only when succeeded are we sure we don't write to it anymore.
	_GFXContext* ret = device->context;

	_gfx_mutex_unlock(&device->lock);

	return ret;
}

/****************************/
_GFXQueueSet* _gfx_pick_queue_set(_GFXContext* context,
                                  VkQueueFlags flags, int present)
{
	assert(context != NULL);

	// Generaly speaking (!?), queue sets only report the flags they were
	// specifically picked for, including the presentation flag.
	// Therefore we just loop over the queue sets and pick the first that
	// satisfies our requirements :)
	for (
		_GFXQueueSet* set = (_GFXQueueSet*)context->sets.head;
		set != NULL;
		set = (_GFXQueueSet*)set->list.next)
	{
		// Check if the required flags and present bit are set.
		if ((set->flags & flags) == flags && (!present || set->present))
			return set;
	}

	// Tough luck.
	return NULL;
}

/****************************/
_GFXQueue _gfx_get_queue(_GFXContext* context,
                         _GFXQueueSet* set, size_t index)
{
	assert(context != NULL);
	assert(set != NULL);
	assert(index < set->count);

	VkQueue queue;
	context->vk.GetDeviceQueue(
		context->vk.device, set->family, (uint32_t)index, &queue);

	return (_GFXQueue){
		.family = set->family,
		.queue = queue,
		.lock = &set->locks[index]
	};
}

/****************************/
GFX_API size_t gfx_get_num_devices(void)
{
	assert(_groufix.initialized);

	return _groufix.devices.size;
}

/****************************/
GFX_API GFXDevice* gfx_get_device(size_t index)
{
	assert(_groufix.initialized);
	assert(_groufix.devices.size > 0);
	assert(index < _groufix.devices.size);

	return gfx_vec_at(&_groufix.devices, index);
}

/****************************/
GFX_API GFXDevice* gfx_get_primary_device(void)
{
	assert(_groufix.initialized);
	assert(_groufix.devices.size > 0);

	return gfx_vec_at(&_groufix.devices, 0);
}
