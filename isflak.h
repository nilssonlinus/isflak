/* isflak.h
 * Author: Linus Nilsson (https://github.com/nilssonlinus)
 * License: MIT
 * Description: A small and simple single header library for initializing
 * Vulkan written in C99.
 */

#ifndef ISFLAK_H
#define ISFLAK_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IK_LOAD_PFN(instance, function_name)                                   \
    PFN_##function_name IK_PFN_##function_name;                                \
    *(void **)&IK_PFN_##function_name =                                        \
        (void *)vkGetInstanceProcAddr(instance, #function_name);

typedef struct ik_instance_info {
    char **extension_names;
    uint32_t extension_count;
    char **validation_layer_names;
    uint32_t validation_layer_count;
    VkBool32 enable_validation_layers;
    char *application_name;
    uint32_t application_version;
    char *engine_name;
    uint32_t engine_version;
    uint32_t api_version;
} ik_instance_info_t;

typedef struct ik_physical_device_info {
    VkInstance instance;
    VkSurfaceKHR surface;
    char **extension_names;
    uint32_t extension_count;
} ik_physical_device_info_t;

typedef struct ik_swapchain_info {
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    VkSharingMode image_sharing_mode;
    uint32_t *queue_family_indices;
    uint32_t queue_family_index_count;
} ik_swapchain_info_t;

typedef struct ik_shader_info {
    VkDevice device;
    VkShaderStageFlagBits shader_stage;
    uint32_t *code;
    size_t code_size;
    char *entry_point;
} ik_shader_info_t;

void ik_create_instance(const ik_instance_info_t *info, VkInstance *instance,
                        VkDebugUtilsMessengerEXT *debug_messenger);

VkPhysicalDevice
ik_choose_physical_device(const ik_physical_device_info_t *info);

void ik_get_queue_family_indices_with_flag(VkPhysicalDevice physical_device,
                                           VkQueueFlagBits queue_flag_bit,
                                           uint32_t **queue_family_indices,
                                           uint32_t *queue_family_index_count);

void ik_get_queue_family_indices_with_surface_support(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    uint32_t **queue_family_indices, uint32_t *queue_family_index_count);

void ik_create_swapchain(const ik_swapchain_info_t *info,
                         VkSwapchainKHR *swapchain, VkImage **images,
                         VkImageView **image_views, uint32_t *image_count);

void ik_create_shader(const ik_shader_info_t *info,
                      VkShaderModule *shader_module,
                      VkPipelineShaderStageCreateInfo *pipeline_shader_stage);

#ifdef ISFLAK_IMPL

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG

#include <assert.h>
#define IK__ASSERT(expression, fail_message)                                   \
    assert(expression && "[ISFLAK]" fail_message);

#define IK__VK_CHECK(result, fail_message)                                     \
    IK__ASSERT(result == VK_SUCCESS, fail_message)

#else

#ifndef ISFLAK_DISABLE_NDEBUG_ASSERT
#define IK__ASSERT(expression, fail_message)                                   \
    if (expression) {                                                          \
    } else {                                                                   \
        fprintf(stderr, "[ISFLAK]: %s\n", fail_message);                       \
    }

#define IK__VK_CHECK(result, fail_message)                                     \
    IK__ASSERT(result == VK_SUCCESS, fail_message)

#endif
#endif

#ifndef IK_CUSTOM_DEBUG_CALLBACK
static VKAPI_ATTR VkBool32 VKAPI_CALL
ik__debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                   VkDebugUtilsMessageTypeFlagsEXT message_type,
                   const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
                   void *p_user_data) {
    fprintf(stderr, "%s\n", p_callback_data->pMessage);
    return VK_FALSE;
}
#endif

void ik_create_instance(const ik_instance_info_t *info, VkInstance *instance,
                        VkDebugUtilsMessengerEXT *debug_messenger) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = info->application_name,
        .applicationVersion = info->application_version,
        .pEngineName = info->engine_name,
        .engineVersion = info->engine_version,
        .apiVersion = info->api_version,
    };

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = info->extension_count,
        .ppEnabledExtensionNames = (const char **)info->extension_names,
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &ik__debug_callback,
    };

    if (info->enable_validation_layers == VK_TRUE) {
        char **extensions = malloc(sizeof(char *));
        if (info->extension_names != NULL) {
            extensions = malloc(sizeof(char *) * (info->extension_count + 1));
            memcpy(extensions, info->extension_names,
                   sizeof(char *) * info->extension_count);
        }
        extensions[info->extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        instance_create_info.ppEnabledLayerNames =
            (const char **)info->validation_layer_names;
        instance_create_info.enabledLayerCount = info->validation_layer_count;
        instance_create_info.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&debug_messenger_create_info;
        instance_create_info.ppEnabledExtensionNames =
            (const char **)extensions;
        instance_create_info.enabledExtensionCount = info->extension_count + 1;
    }

    IK__VK_CHECK(vkCreateInstance(&instance_create_info, NULL, instance),
                 "Instance creation failed.");

    if (info->enable_validation_layers == VK_TRUE) {
        IK_LOAD_PFN(*instance, vkCreateDebugUtilsMessengerEXT);
        IK__VK_CHECK(
            IK_PFN_vkCreateDebugUtilsMessengerEXT(
                *instance, &debug_messenger_create_info, NULL, debug_messenger),
            "Debug messenger creation failed.");
    }
}

VkPhysicalDevice
ik_choose_physical_device(const ik_physical_device_info_t *info) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(info->instance, &device_count, NULL);
    IK__ASSERT(device_count != 0,
               "Couldn't find any physical device with Vulkan support.");

    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(info->instance, &device_count, devices);

    VkPhysicalDevice physical_device = NULL;

    for (uint32_t i = 0; i < device_count; i++) {
        VkBool32 surface_support;
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i],
                                                 &queue_family_count, NULL);
        for (uint32_t queue_family_index = 0;
             queue_family_index < queue_family_count; queue_family_count++) {
            vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], queue_family_index,
                                                 info->surface,
                                                 &surface_support);
            if (surface_support == VK_TRUE) {
                break;
            }
        }

        VkBool32 extension_support;
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(devices[i], NULL, &extension_count,
                                             NULL);

        VkExtensionProperties *available_extensions =
            malloc(sizeof(VkExtensionProperties) * extension_count);
        vkEnumerateDeviceExtensionProperties(devices[i], NULL, &extension_count,
                                             available_extensions);

        uint32_t supported_extension_count = 0;
        for (uint32_t available_extension = 0;
             available_extension < extension_count; available_extension++) {
            for (uint32_t device_extension = 0;
                 device_extension < info->extension_count; device_extension++) {
                if (strcmp(
                        available_extensions[available_extension].extensionName,
                        info->extension_names[device_extension]) == 0) {
                    supported_extension_count += 1;
                }
            }
        }
        free(available_extensions);

        if (supported_extension_count == info->extension_count) {
            extension_support = VK_TRUE;
        }

        if (surface_support == VK_TRUE && extension_support == VK_TRUE) {
            physical_device = devices[i];
            break;
        }
    }
    free(devices);
    IK__ASSERT(physical_device != NULL,
               "No physical device with extension support found.");

    return physical_device;
}

void ik_get_queue_family_indices_with_flag(VkPhysicalDevice physical_device,
                                           VkQueueFlagBits queue_flag_bit,
                                           uint32_t **queue_family_indices,
                                           uint32_t *queue_family_index_count) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device,
                                             &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_families =
        malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, queue_families);

    uint32_t *family_indices = malloc(sizeof(uint32_t));
    uint32_t found = 0;

    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & queue_flag_bit) {
            family_indices[found] = i;
            found += 1;
            family_indices =
                realloc(family_indices, sizeof(uint32_t) * (found + 1));
        }
    }

    free(queue_families);
    IK__ASSERT(family_indices != NULL,
               "Couldn't find queue family indices with flag.");
    if (queue_family_indices != NULL) {
        *queue_family_indices = family_indices;
    }
    if (queue_family_index_count != NULL) {
        *queue_family_index_count = queue_family_count;
    }
}

void ik_get_queue_family_indices_with_surface_support(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    uint32_t **queue_family_indices, uint32_t *queue_family_index_count) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device,
                                             &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_families =
        malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, queue_families);

    uint32_t *family_indices = malloc(sizeof(uint32_t));
    uint32_t found = 0;

    for (uint32_t i = 0; i < queue_family_count; i++) {
        VkBool32 present_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface,
                                             &present_support);
        if (present_support) {
            family_indices[found] = i;
            found += 1;
            family_indices =
                realloc(family_indices, sizeof(uint32_t) * (found + 1));
        }
    }

    free(queue_families);
    IK__ASSERT(family_indices != NULL,
               "Couldn't find queue family indices with surface support.");
    if (queue_family_indices != NULL) {
        *queue_family_indices = family_indices;
    }
    if (queue_family_index_count != NULL) {
        *queue_family_index_count = queue_family_count;
    }
}

void ik_create_swapchain(const ik_swapchain_info_t *info,
                         VkSwapchainKHR *swapchain, VkImage **images,
                         VkImageView **image_views, uint32_t *image_count) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info->physical_device,
                                              info->surface, &capabilities);

    VkSurfaceFormatKHR surface_format;
    uint32_t format_count;

    vkGetPhysicalDeviceSurfaceFormatsKHR(info->physical_device, info->surface,
                                         &format_count, NULL);
    VkSurfaceFormatKHR *available_formats =
        malloc(sizeof(VkSurfaceFormatKHR) * format_count);

    vkGetPhysicalDeviceSurfaceFormatsKHR(info->physical_device, info->surface,
                                         &format_count, available_formats);

    for (uint32_t i = 0; i < format_count; i++) {
        if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_formats[i].colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = available_formats[i];
        }
    }

    VkPresentModeKHR present_mode;
    uint32_t present_mode_count;
    VkBool32 mailbox = VK_FALSE;

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        info->physical_device, info->surface, &present_mode_count, NULL);

    VkPresentModeKHR *available_present_modes =
        malloc(sizeof(VkPresentModeKHR) * present_mode_count);

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        info->physical_device, info->surface, &present_mode_count,
        available_present_modes);

    for (uint32_t i = 0; i < present_mode_count; i++) {
        if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR &&
            mailbox == VK_FALSE) {
            present_mode = available_present_modes[i];
            mailbox = VK_TRUE;
        }
    }
    if (mailbox == VK_FALSE) {
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent = info->extent;
        if (extent.width > capabilities.maxImageExtent.width) {
            extent.width = capabilities.maxImageExtent.width;
        }
        if (extent.height > capabilities.maxImageExtent.height) {
            extent.height = capabilities.maxImageExtent.height;
        }
    }

    uint32_t min_image_count = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 &&
        min_image_count > capabilities.maxImageCount) {
        min_image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = info->surface,
        .minImageCount = min_image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = info->image_sharing_mode,
        .queueFamilyIndexCount = info->queue_family_index_count,
        .pQueueFamilyIndices = info->queue_family_indices,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    free(available_formats);
    free(available_present_modes);

    vkCreateSwapchainKHR(info->device, &swapchain_create_info, NULL, swapchain);

    uint32_t swapchain_image_count;
    vkGetSwapchainImagesKHR(info->device, *swapchain, &swapchain_image_count,
                            NULL);
    VkImage *swapchain_images = malloc(sizeof(VkImage) * swapchain_image_count);
    vkGetSwapchainImagesKHR(info->device, *swapchain, &swapchain_image_count,
                            swapchain_images);

    if (image_count != NULL) {
        *image_count = swapchain_image_count;
    }

    if (images != NULL) {
        *images = swapchain_images;
    }

    VkImageView *swapchain_image_views =
        malloc(sizeof(VkImageView) * swapchain_image_count);

    for (uint32_t i = 0; i < swapchain_image_count; i++) {
        VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surface_format.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        VkImageView image_view;
        IK__VK_CHECK(vkCreateImageView(info->device, &image_view_create_info,
                                       NULL, &image_view),
                     "Swapchain image creation failed.");
        swapchain_image_views[i] = image_view;
    }

    if (image_views != NULL) {
        *image_views = swapchain_image_views;
    }
}

void ik_create_shader(const ik_shader_info_t *info,
                      VkShaderModule *shader_module,
                      VkPipelineShaderStageCreateInfo *pipeline_shader_stage) {
    const VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = info->code_size,
        .pCode = info->code,
    };
    vkCreateShaderModule(info->device, &shader_module_create_info, NULL,
                         shader_module);

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = info->shader_stage,
        .module = *shader_module,
        .pName = info->entry_point,
    };

    if (pipeline_shader_stage != NULL) {
        *pipeline_shader_stage = pipeline_shader_stage_create_info;
    }
}
#endif

#ifdef __cplusplus
}
#endif

#endif
