#ifndef ISFLAK_H
#define ISFLAK_H

#include <vulkan/vulkan_core.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IK_LOAD_VK_PFN(instance, function_name)                                \
    PFN_##function_name VK_PFN_##function_name;                                \
    *(void **)&VK_PFN_##function_name =                                        \
        (void *)vkGetInstanceProcAddr(instance, #function_name);

#define IK_VALIDATION_LAYERS                                                   \
    { "VK_LAYER_KHRONOS_validation" }

typedef struct IkInstanceInfo {
    char **extensions;
    uint32_t extension_count;
    VkBool32 enable_validation_layers;
    char *application_name;
    uint32_t application_version;
    char *engine_name;
    uint32_t engine_version;
    uint32_t api_version;
} IkInstanceInfo;

VkInstance ik_create_instance(IkInstanceInfo *info,
                              VkDebugUtilsMessengerEXT *debug_messenger);
VkPhysicalDevice ik_choose_physical_device();
VkDevice ik_create_device();

#ifdef IK_IMPLEMENTATION

#include <stdio.h>
#include <assert.h>

#define _IK_VK_CHECK(result, message)                                          \
    assert(result == VK_SUCCESS && "[IK]" message);                            \
    if (result != VK_SUCCESS)                                                  \
        fprintf(stderr, "[IK]" message "failed. \n");

#ifndef IK_CUSTOM_DEBUG_CALLBACK
static VKAPI_ATTR VkBool32 VKAPI_CALL _IK_VK_DEBUG_CALLBACK(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
    void *p_user_data) {
    fprintf(stderr, "%s\n", p_callback_data->pMessage);
    return VK_FALSE;
}
#endif

VkInstance ik_create_instance(IkInstanceInfo *info,
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
        .flags = NULL,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = info->extension_count,
        .ppEnabledExtensionNames = info->extensions,
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &_IK_VK_DEBUG_CALLBACK,
    };

    if (info->enable_validation_layers == VK_TRUE) {
        char **extensions = info->extensions;
        extensions[info->extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        const char *layer_names[] = IK_VALIDATION_LAYERS;
        instance_create_info.ppEnabledLayerNames = layer_names;
        instance_create_info.enabledLayerCount =
            sizeof(layer_names) / sizeof(layer_names[0]);
        instance_create_info.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&debug_messenger_create_info;
        instance_create_info.ppEnabledExtensionNames = extensions;
        instance_create_info.enabledExtensionCount = info->extension_count + 1;

        printf("TEMP! Enabled layer count: %d\n",
               instance_create_info.enabledLayerCount);
    }

    VkInstance instance;
    _IK_VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &instance),
                 "Instance creation");

    if (info->enable_validation_layers == VK_TRUE) {
        IK_LOAD_VK_PFN(instance, vkCreateDebugUtilsMessengerEXT);
        _IK_VK_CHECK(
            VK_PFN_vkCreateDebugUtilsMessengerEXT(
                instance, &debug_messenger_create_info, NULL, debug_messenger),
            "Debug messenger creation")
    }

    return instance;
}

#endif

#ifdef __cplusplus
}
#endif

#endif
