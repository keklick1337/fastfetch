#include "common/printing.h"
#include "common/jsonconfig.h"
#include "detection/gamepad/gamepad.h"
#include "modules/gamepad/gamepad.h"

#define FF_GAMEPAD_NUM_FORMAT_ARGS 2

static void printDevice(FFinstance* instance, FFGamepadOptions* options, const FFGamepadDevice* device, uint8_t index)
{
    if(options->moduleArgs.outputFormat.length == 0)
    {
        ffPrintLogoAndKey(instance, FF_GAMEPAD_MODULE_NAME, index, &options->moduleArgs.key);
        ffStrbufPutTo(&device->name, stdout);
    }
    else
    {
        ffPrintFormat(instance, FF_GAMEPAD_MODULE_NAME, index, &options->moduleArgs, FF_GAMEPAD_NUM_FORMAT_ARGS, (FFformatarg[]) {
            {FF_FORMAT_ARG_TYPE_STRBUF, &device->name},
            {FF_FORMAT_ARG_TYPE_STRBUF, &device->identifier},
        });
    }
}

void ffPrintGamepad(FFinstance* instance, FFGamepadOptions* options)
{
    FF_LIST_AUTO_DESTROY result = ffListCreate(sizeof(FFGamepadDevice));

    const char* error = ffDetectGamepad(instance, &result);

    if(error)
    {
        ffPrintError(instance, FF_GAMEPAD_MODULE_NAME, 0, &options->moduleArgs, "%s", error);
        return;
    }

    if(!result.length)
    {
        ffPrintError(instance, FF_GAMEPAD_MODULE_NAME, 0, &options->moduleArgs, "No devices detected");
        return;
    }

    uint8_t index = 0;
    FF_LIST_FOR_EACH(FFGamepadDevice, device, result)
    {
        printDevice(instance, options, device, result.length > 1 ? ++index : 0);
        ffStrbufDestroy(&device->identifier);
        ffStrbufDestroy(&device->name);
    }
}

void ffInitGamepadOptions(FFGamepadOptions* options)
{
    options->moduleName = FF_GAMEPAD_MODULE_NAME;
    ffOptionInitModuleArg(&options->moduleArgs);
}

bool ffParseGamepadCommandOptions(FFGamepadOptions* options, const char* key, const char* value)
{
    const char* subKey = ffOptionTestPrefix(key, FF_GAMEPAD_MODULE_NAME);
    if (!subKey) return false;
    if (ffOptionParseModuleArgs(key, subKey, value, &options->moduleArgs))
        return true;

    return false;
}

void ffDestroyGamepadOptions(FFGamepadOptions* options)
{
    ffOptionDestroyModuleArg(&options->moduleArgs);
}

void ffParseGamepadJsonObject(FFinstance* instance, yyjson_val* module)
{
    FFGamepadOptions __attribute__((__cleanup__(ffDestroyGamepadOptions))) options;
    ffInitGamepadOptions(&options);

    if (module)
    {
        yyjson_val *key_, *val;
        size_t idx, max;
        yyjson_obj_foreach(module, idx, max, key_, val)
        {
            const char* key = yyjson_get_str(key_);
            if(strcasecmp(key, "type") == 0)
                continue;

            if (ffJsonConfigParseModuleArgs(key, val, &options.moduleArgs))
                continue;

            ffPrintError(instance, FF_GAMEPAD_MODULE_NAME, 0, &options.moduleArgs, "Unknown JSON key %s", key);
        }
    }

    ffPrintGamepad(instance, &options);
}