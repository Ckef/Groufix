/**
 * This file is part of groufix.
 * Copyright (c) Stef Velzel. All rights reserved.
 *
 * groufix : graphics engine produced by Stef Velzel.
 * www     : <www.vuzzel.nl>
 */


#ifndef GFX_CORE_DEVICE_H
#define GFX_CORE_DEVICE_H

#include "groufix/def.h"


/**
 * Physical device type.
 * From most preferred to least preferred.
 */
typedef enum GFXDeviceType
{
	GFX_DEVICE_DISCRETE_GPU,
	GFX_DEVICE_VIRTUAL_GPU,
	GFX_DEVICE_INTEGRATED_GPU,
	GFX_DEVICE_CPU,
	GFX_DEVICE_UNKNOWN

} GFXDeviceType;


/**
 * Physical device definition (e.g. a GPU).
 */
typedef struct GFXDevice
{
	GFXDeviceType type; // Read-only.
	const char*   name; // Read-only.

} GFXDevice;


/**
 * Retrieves the number of initialized devices.
 * @return 0 if no devices were found.
 */
GFX_API size_t gfx_get_num_devices(void);

/**
 * Retrieves an initialized device.
 * The primary device is always stored at index 0 and stays constant.
 * @param index Must be < gfx_get_num_devices().
 */
GFX_API GFXDevice* gfx_get_device(size_t index);

/**
 * Retrieves the primary device.
 * This is equivalent to gfx_get_device(0).
 */
GFX_API GFXDevice* gfx_get_primary_device(void);


#endif
