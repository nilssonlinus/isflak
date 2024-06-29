/* isflak.h
 * Author: Linus Nilsson (https://github.com/nilssonlinus)
 * License: MIT
 * Description: A single header Vulkan helper library.
 */

#ifndef ISFLAK_H
#define ISFLAK_H

#include <vulkan/vulkan_core.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IK_LOAD_PFN(instance, function_name)                                   \
    PFN_##function_name IK_PFN_##function_name;                                \
    *(void **)&IK_PFN_##function_name =                                        \
        (void *)vkGetInstanceProcAddr(instance, #function_name);

typedef struct ik_instance_info {
    const char **extension_names;
    uint32_t extension_count;
    const char **validation_layer_names;
    uint32_t validation_layer_count;
    VkBool32 enable_validation_layers;
    char *application_name;
    uint32_t application_version;
    char *engine_name;
    uint32_t engine_version;
    uint32_t api_version;
} ik_instance_info;

typedef struct ik_physical_device_info {
    VkInstance instance;
    VkSurfaceKHR surface;
    char **extension_names;
    uint32_t extension_count;
} ik_physical_device_info;

VkInstance ik_create_instance(ik_instance_info *info,
                              VkDebugUtilsMessengerEXT *debug_messenger);

VkPhysicalDevice ik_choose_physical_device(ik_physical_device_info *info);

#ifdef ISFLAK_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define IK__ASSERT_EQ(value, success_state, message)                           \
    assert(value == success_state && "[ISFLAK]" message);                      \
    if (value != success_state)                                                \
        fprintf(stderr, "[ISFLAK] Assertion" message "failed. \n");

#define IK__ASSERT_NEQ(value, fail_state, message)                             \
    assert(value != fail_state && "[ISFLAK]" message);                         \
    if (value == fail_state)                                                   \
        fprintf(stderr, "[ISFLAK] Assertion" message "failed. \n");

#define IK__VK_CHECK(result, message) IK__ASSERT_EQ(result, VK_SUCCESS, message)

#ifndef IK_CUSTOM_DEBUG_CALLBACK
static VKAPI_ATTR VkBool32 VKAPI_CALL
IK__DEBUG_CALLBACK(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                   VkDebugUtilsMessageTypeFlagsEXT message_type,
                   const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
                   void *p_user_data) {
    fprintf(stderr, "%s\n", p_callback_data->pMessage);
    return VK_FALSE;
}
#endif

VkInstance ik_create_instance(ik_instance_info *info,
                              VkDebugUtilsMessengerEXT *debug_messenger) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = info->application_name,
        .applicationVersion = info->application_version,
        .pEngineName = info->engine_name,
        .engineVersion = info->engine_version,
        .apiVersion = info->api_version,
    };

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = info->extension_count,
        .ppEnabledExtensionNames = info->extension_names,
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &IK__DEBUG_CALLBACK,
    };

    if (info->enable_validation_layers == VK_TRUE) {
        char **extensions = (char **)malloc(sizeof(char *));
        if (info->extension_names != NULL) {
            extensions =
                (char **)malloc(sizeof(char *) * (info->extension_count + 1));
            memcpy(extensions, info->extension_names,
                   sizeof(char *) * info->extension_count);
        }
        extensions[info->extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        instance_create_info.ppEnabledLayerNames = info->validation_layer_names;
        instance_create_info.enabledLayerCount = info->validation_layer_count;
        instance_create_info.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&debug_messenger_create_info;
        instance_create_info.ppEnabledExtensionNames =
            (const char **)extensions;
        instance_create_info.enabledExtensionCount = info->extension_count + 1;
    }

    VkInstance instance;
    IK__VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &instance),
                 "Instance creation failed");

    if (info->enable_validation_layers == VK_TRUE) {
        IK_LOAD_PFN(instance, vkCreateDebugUtilsMessengerEXT);
        IK__VK_CHECK(
            IK_PFN_vkCreateDebugUtilsMessengerEXT(
                instance, &debug_messenger_create_info, NULL, debug_messenger),
            "Debug messenger creation failed");
    }

    return instance;
}

VkPhysicalDevice ik_choose_physical_device(ik_physical_device_info *info) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(info->instance, &device_count, NULL);
    IK__ASSERT_NEQ(device_count, 0, "Physical device count is zero");

    VkPhysicalDevice *devices =
        (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(info->instance, &device_count, devices);

    VkPhysicalDevice physical_device = NULL;

    for (uint32_t current_device = 0; current_device < device_count;
         current_device++) {
        VkBool32 surface_support;
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[current_device],
                                                 &queue_family_count, NULL);
        for (uint32_t queue_family_index = 0;
             queue_family_index < queue_family_count; queue_family_count++) {
            vkGetPhysicalDeviceSurfaceSupportKHR(
                devices[current_device], queue_family_index, info->surface,
                &surface_support);
            if (surface_support == VK_TRUE) {
                break;
            }
        }

        VkBool32 extension_support;
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(devices[current_device], NULL,
                                             &extension_count, NULL);

        VkExtensionProperties *available_extensions =
            (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) *
                                            extension_count);
        vkEnumerateDeviceExtensionProperties(devices[current_device], NULL,
                                             &extension_count,
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
            physical_device = devices[current_device];
            break;
        }
    }
    free(devices);
    IK__ASSERT_NEQ(physical_device, NULL, "Physical device not found");

    return physical_device;
}

#endif

#ifdef __cplusplus
}
#endif

#endif