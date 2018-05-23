//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Data Hub Query API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "dataHub.h"
#include "handler.h"


//--------------------------------------------------------------------------------------------------
/**
 * Find an Observation.
 *
 * @return Reference to the Observation's resource tree entry, or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
static resTree_EntryRef_t FindObservation
(
    const char* path
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entry;

    if (strncmp(path, "/obs/", 5) == 0)
    {
        entry = resTree_FindEntry(resTree_GetRoot(), path);
    }
    else if (path[0] == '/')
    {
        return NULL;
    }
    else
    {
        resTree_EntryRef_t obsNamespace = resTree_FindEntry(resTree_GetRoot(), "obs");

        if (obsNamespace == NULL)
        {
            return NULL;
        }

        entry = resTree_FindEntry(obsNamespace, path);
    }

    if (entry == NULL)
    {
        return NULL;
    }

    if (resTree_GetEntryType(entry) != ADMIN_ENTRY_TYPE_OBSERVATION)
    {
        return NULL;
    }

    return entry;
}


//--------------------------------------------------------------------------------------------------
/**
 * Read data out of a buffer.  Data is written to a given file descriptor in JSON-encoded format
 * as an array of objects containing a timestamp and a value (or just a timestamp for triggers).
 * E.g.,
 *
 * @code
 * [{"t":1537483647.125,"v":true},{"t":1537483657.128,"v":true}]
 * @endcode
 *
 * @return
 *  - LE_OK if the read operation started successfully.
 *  - LE_NOT_FOUND if the Observation doesn't exist.
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_ReadBufferJson
(
    const char* obsPath,
        ///< [IN] Observation path. Can be absolute
        ///< (beginning with a '/') or relative to /obs/.
    double startAfter,
        ///< [IN] Start after this many seconds ago,
        ///< or after an absolute number of seconds since the Epoch
        ///< (if startafter > 30 years).
        ///< Use NAN (not a number) to read the whole buffer.
    int outputFile,
        ///< [IN] File descriptor to write the data to.
    query_ReadCompletionFunc_t completionFuncPtr,
        ///< [IN] Completion callback to be called when operation finishes.
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindObservation(obsPath);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    if (startAfter < 0)
    {
        LE_KILL_CLIENT("Negative startAfter time provided (%lf).", startAfter);
        return LE_OK;   // Doesn't matter what we return.
    }

    resTree_ReadBufferJson(entryRef, startAfter, outputFile, completionFuncPtr, contextPtr);

    return LE_OK;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the minimum value found in an Observation's data set withing a given time span.
 *
 * @return The value, or NAN (not-a-number) if there's no numerical data in the Observation's
 *         buffer (if the buffer size is zero, the buffer is empty, or the buffer contains data
 *         of a non-numerical type).
 */
//--------------------------------------------------------------------------------------------------
double query_GetMin
(
    const char* obsPath,
        ///< [IN] Observation path. Can be absolute
        ///< (beginning with a '/') or relative to /obs/.
    double startTime
        ///< [IN] If < 30 years then seconds before now; else seconds since the Epoch.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindObservation(obsPath);

    if (entryRef == NULL)
    {
        return NAN;
    }

    return NAN;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the maximum value found within a given time span in an Observation's buffer.
 *
 * @return The value, or NAN (not-a-number) if there's no numerical data in the Observation's
 *         buffer (if the buffer size is zero, the buffer is empty, or the buffer contains data
 *         of a non-numerical type).
 */
//--------------------------------------------------------------------------------------------------
double query_GetMax
(
    const char* obsPath,
        ///< [IN] Observation path. Can be absolute
        ///< (beginning with a '/') or relative to /obs/.
    double startTime
        ///< [IN] If < 30 years then seconds before now; else seconds since the Epoch.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindObservation(obsPath);

    if (entryRef == NULL)
    {
        return NAN;
    }

    return NAN;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the mean (average) of all values found within a given time span in an Observation's buffer.
 *
 * @return The value, or NAN (not-a-number) if there's no numerical data in the Observation's
 *         buffer (if the buffer size is zero, the buffer is empty, or the buffer contains data
 *         of a non-numerical type).
 */
//--------------------------------------------------------------------------------------------------
double query_GetMean
(
    const char* obsPath,
        ///< [IN] Observation path. Can be absolute
        ///< (beginning with a '/') or relative to /obs/.
    double startTime
        ///< [IN] If < 30 years then seconds before now; else seconds since the Epoch.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindObservation(obsPath);

    if (entryRef == NULL)
    {
        return NAN;
    }

    return NAN;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the standard deviation of all values found within a given time span in an
 * Observation's buffer.
 *
 * @return The value, or NAN (not-a-number) if there's no numerical data in the Observation's
 *         buffer (if the buffer size is zero, the buffer is empty, or the buffer contains data
 *         of a non-numerical type).
 */
//--------------------------------------------------------------------------------------------------
double query_GetStdDev
(
    const char* obsPath,
        ///< [IN] Observation path. Can be absolute
        ///< (beginning with a '/') or relative to /obs/.
    double startTime
        ///< [IN] If < 30 years then seconds before now; else seconds since the Epoch.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindObservation(obsPath);

    if (entryRef == NULL)
    {
        return NAN;
    }

    return NAN;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a resource at a given path.  The path can be absolute (beginning with a '/'), or relative
 * to the calling app's namespace.
 *
 * @return Reference to the resource or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
static resTree_EntryRef_t FindResource
(
    const char* path
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef;

    if (path[0] == '/')
    {
        entryRef = resTree_FindEntryAtAbsolutePath(path);
    }
    else
    {
        entryRef = hub_GetClientNamespace(query_GetClientSessionRef());

        if (entryRef != NULL)
        {
            entryRef = resTree_FindEntry(entryRef, path);

            if ((entryRef != NULL) && (!resTree_IsResource(entryRef)))
            {
                LE_DEBUG("Entry '%s' is not a resource (it's a %s).",
                         path,
                         hub_GetEntryTypeName(resTree_GetEntryType(entryRef)));
                entryRef = NULL;
            }
        }
    }

    return entryRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the current data type of a resource.
 *
 * @note Observations and Placeholders will default to IO_DATA_TYPE_TRIGGER, but will change
 *       types as other types of data are pushed to them.  The data types of Inputs and Outputs
 *       are decided by the apps that create them.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource was not found.
 *  - LE_UNSUPPORTED if the path refers to a namespace (which can't have a data type).
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_GetDataType
(
    const char* path,
        ///< [IN] Resource path. Can be absolute (beginning
        ///< with a '/') or relative to the namespace of
        ///< the calling app (/app/<app-name>/).
    io_DataType_t* dataTypePtr
        ///< [OUT] The fetched data type, if LE_OK is returned.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindResource(path);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }
    else if (!resTree_IsResource(entryRef))
    {
        return LE_UNSUPPORTED;
    }
    else
    {
        *dataTypePtr = resTree_GetDataType(entryRef);
        return LE_OK;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the current units of a resource.
 *
 * @note Observations and Placeholders will default to "", but will change units as data is
 *       pushed to them.  The units of Inputs and Outputs are decided by the apps that create them.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource was not found.
 *  - LE_UNSUPPORTED if the path refers to a namespace (which can't have a data type).
 *  - LE_OVERFLOW if the units string was truncated because it is larger than the buffer provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_GetUnits
(
    const char* path,
        ///< [IN] Resource path. Can be absolute (beginning
        ///< with a '/') or relative to the namespace of
        ///< the calling app (/app/<app-name>/).
    char* units,
        ///< [OUT] The fetched units, if LE_OK is returned.
    size_t unitsSize
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindResource(path);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }
    else if (!resTree_IsResource(entryRef))
    {
        return LE_UNSUPPORTED;
    }
    else
    {
        return le_utf8_Copy(units, resTree_GetUnits(entryRef), unitsSize, NULL);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the timestamp of the current value of a resource.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource was not found.
 *  - LE_UNSUPPORTED if the path refers to a namespace (which can't have a data type).
 *  - LE_UNAVAILABLE if the resource doesn't have a current value (yet).
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_GetTimestamp
(
    const char* path,
        ///< [IN] Resource path. Can be absolute (beginning
        ///< with a '/') or relative to the namespace of
        ///< the calling app (/app/<app-name>/).
    double* timestampPtr
        ///< [OUT] The fetched timestamp (in seconds since the Epoch), if LE_OK returned.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindResource(path);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }
    else if (!resTree_IsResource(entryRef))
    {
        return LE_UNSUPPORTED;
    }
    else
    {
        dataSample_Ref_t sampleRef = resTree_GetCurrentValue(entryRef);

        if (sampleRef == NULL)
        {
            return LE_UNAVAILABLE;
        }
        else
        {
            *timestampPtr = dataSample_GetTimestamp(sampleRef);

            return LE_OK;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the current value of a resource, if it's Boolean type.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource was not found.
 *  - LE_UNSUPPORTED if the path refers to a namespace (which can't have a data type).
 *  - LE_UNAVAILABLE if the resource doesn't have a current value (yet).
 *  - LE_FORMAT_ERROR if the resource has another data type.
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_GetBoolean
(
    const char* path,
        ///< [IN] Resource path. Can be absolute (beginning
        ///< with a '/') or relative to the namespace of
        ///< the calling app (/app/<app-name>/).
    double* timestampPtr,
        ///< [OUT] Fetched timestamp (in seconds since the Epoch), if LE_OK returned.
    bool* valuePtr
        ///< [OUT] Fetched value, if LE_OK returned.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindResource(path);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }
    else if (!resTree_IsResource(entryRef))
    {
        return LE_UNSUPPORTED;
    }
    else
    {
        dataSample_Ref_t sampleRef = resTree_GetCurrentValue(entryRef);

        if (sampleRef == NULL)
        {
            return LE_UNAVAILABLE;
        }
        else if (resTree_GetDataType(entryRef) != IO_DATA_TYPE_BOOLEAN)
        {
            return LE_FORMAT_ERROR;
        }
        else
        {
            *timestampPtr = dataSample_GetTimestamp(sampleRef);

            *valuePtr = dataSample_GetBoolean(sampleRef);

            return LE_OK;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the current value of a resource, if it's numeric type.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource was not found.
 *  - LE_UNSUPPORTED if the path refers to a namespace (which can't have a data type).
 *  - LE_UNAVAILABLE if the resource doesn't have a current value (yet).
 *  - LE_FORMAT_ERROR if the resource has another data type.
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_GetNumeric
(
    const char* path,
        ///< [IN] Resource path. Can be absolute (beginning
        ///< with a '/') or relative to the namespace of
        ///< the calling app (/app/<app-name>/).
    double* timestampPtr,
        ///< [OUT] Fetched timestamp (in seconds since the Epoch), if LE_OK returned.
    double* valuePtr
        ///< [OUT] Fetched value, if LE_OK returned.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindResource(path);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }
    else if (!resTree_IsResource(entryRef))
    {
        return LE_UNSUPPORTED;
    }
    else
    {
        dataSample_Ref_t sampleRef = resTree_GetCurrentValue(entryRef);

        if (sampleRef == NULL)
        {
            return LE_UNAVAILABLE;
        }
        else if (resTree_GetDataType(entryRef) != IO_DATA_TYPE_NUMERIC)
        {
            return LE_FORMAT_ERROR;
        }
        else
        {
            *timestampPtr = dataSample_GetTimestamp(sampleRef);

            *valuePtr = dataSample_GetNumeric(sampleRef);

            return LE_OK;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the current value of a resource, if it's a string type.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource was not found.
 *  - LE_UNSUPPORTED if the path refers to a namespace (which can't have a data type).
 *  - LE_UNAVAILABLE if the resource doesn't have a current value (yet).
 *  - LE_FORMAT_ERROR if the resource has another data type.
 *  - LE_OVERFLOW if the value was truncated because it is larger than the buffer provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_GetString
(
    const char* path,
        ///< [IN] Resource path. Can be absolute (beginning
        ///< with a '/') or relative to the namespace of
        ///< the calling app (/app/<app-name>/).
    double* timestampPtr,
        ///< [OUT] Fetched timestamp (in seconds since the Epoch), if LE_OK returned.
    char* value,
        ///< [OUT] Fetched value, if LE_OK returned.
    size_t valueSize
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindResource(path);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }
    else if (!resTree_IsResource(entryRef))
    {
        return LE_UNSUPPORTED;
    }
    else
    {
        dataSample_Ref_t sampleRef = resTree_GetCurrentValue(entryRef);

        if (sampleRef == NULL)
        {
            return LE_UNAVAILABLE;
        }
        else if (resTree_GetDataType(entryRef) != IO_DATA_TYPE_STRING)
        {
            return LE_FORMAT_ERROR;
        }
        else
        {
            *timestampPtr = dataSample_GetTimestamp(sampleRef);

            return le_utf8_Copy(value, dataSample_GetString(sampleRef), valueSize, NULL);
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the current value of a resource of any type, in JSON format.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource was not found.
 *  - LE_UNSUPPORTED if the path refers to a namespace (which can't have a data type).
 *  - LE_UNAVAILABLE if the resource doesn't have a current value (yet).
 *  - LE_OVERFLOW if the value was truncated because it is larger than the buffer provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_GetJson
(
    const char* path,
        ///< [IN] Resource path. Can be absolute (beginning
        ///< with a '/') or relative to the namespace of
        ///< the calling app (/app/<app-name>/).
    double* timestampPtr,
        ///< [OUT] Fetched timestamp (in seconds since the Epoch), if LE_OK returned.
    char* value,
        ///< [OUT] Fetched value, if LE_OK returned.
    size_t valueSize
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindResource(path);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }
    else if (!resTree_IsResource(entryRef))
    {
        return LE_UNSUPPORTED;
    }
    else
    {
        dataSample_Ref_t sampleRef = resTree_GetCurrentValue(entryRef);

        if (sampleRef == NULL)
        {
            return LE_UNAVAILABLE;
        }
        else
        {
            *timestampPtr = dataSample_GetTimestamp(sampleRef);

            io_DataType_t dataType = resTree_GetDataType(entryRef);

            if (dataType == IO_DATA_TYPE_JSON)
            {
                return le_utf8_Copy(value, dataSample_GetJson(sampleRef), valueSize, NULL);
            }
            else
            {
                return dataSample_ConvertToJson(sampleRef, dataType, value, valueSize);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetch the example JSON value string for a given Input resource.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource was not found.
 *  - LE_UNSUPPORTED if the path refers to something that doesn't have a JSON type.
 *  - LE_UNAVAILABLE if the JSON-type resource doesn't have an example value.
 *  - LE_OVERFLOW if the value was truncated because it is larger than the buffer provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t query_GetJsonExample
(
    const char* path,
        ///< [IN] Resource path. Can be absolute (beginning
        ///< with a '/') or relative to the namespace of
        ///< the calling app (/app/<app-name>/).
    char* example,
        ///< [OUT] Example will be put here, if LE_OK returned.
    size_t exampleSize
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = FindResource(path);

    if (entryRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    if (!resTree_IsResource(entryRef) || (resTree_GetDataType(entryRef) != IO_DATA_TYPE_JSON))
    {
        return LE_UNSUPPORTED;
    }

    dataSample_Ref_t sampleRef = resTree_GetJsonExample(entryRef);

    if (sampleRef != NULL)
    {
        return le_utf8_Copy(example, dataSample_GetJson(sampleRef), exampleSize, NULL);
    }

    return LE_UNAVAILABLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a handler function to be called when a value is pushed to (and accepted by) a resource
 * anywhere in the resource tree.  If there's no resource at that location yet, a placeholder will
 * be created.
 *
 * @return A reference to the handler, which can be removed using handler_Remove().
 */
//--------------------------------------------------------------------------------------------------
static hub_HandlerRef_t AddPushHandler
(
    const char* path,   ///< Absolute resource path.
    io_DataType_t dataType,
    void* callbackPtr,  ///< Callback function pointer
    void* contextPtr
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = resTree_GetResource(resTree_GetRoot(), path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Bad resource path '%s'.", path);
        return NULL;
    }

    return resTree_AddPushHandler(resRef, dataType, callbackPtr, contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'query_TriggerPush'
 */
//--------------------------------------------------------------------------------------------------
query_TriggerPushHandlerRef_t query_AddTriggerPushHandler
(
    const char* path,
        ///< [IN] Absolute path of resource.
    query_TriggerPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    hub_HandlerRef_t ref = AddPushHandler(path, IO_DATA_TYPE_TRIGGER, callbackPtr, contextPtr);

    return (query_TriggerPushHandlerRef_t)ref;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'query_TriggerPush'
 */
//--------------------------------------------------------------------------------------------------
void query_RemoveTriggerPushHandler
(
    query_TriggerPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    handler_Remove((hub_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'query_BooleanPush'
 */
//--------------------------------------------------------------------------------------------------
query_BooleanPushHandlerRef_t query_AddBooleanPushHandler
(
    const char* path,
        ///< [IN] Absolute path of resource.
    query_BooleanPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    hub_HandlerRef_t ref = AddPushHandler(path, IO_DATA_TYPE_BOOLEAN, callbackPtr, contextPtr);

    return (query_BooleanPushHandlerRef_t)ref;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'query_BooleanPush'
 */
//--------------------------------------------------------------------------------------------------
void query_RemoveBooleanPushHandler
(
    query_BooleanPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    handler_Remove((hub_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'query_NumericPush'
 */
//--------------------------------------------------------------------------------------------------
query_NumericPushHandlerRef_t query_AddNumericPushHandler
(
    const char* path,
        ///< [IN] Absolute path of resource.
    query_NumericPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    hub_HandlerRef_t ref = AddPushHandler(path, IO_DATA_TYPE_NUMERIC, callbackPtr, contextPtr);

    return (query_NumericPushHandlerRef_t)ref;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'query_NumericPush'
 */
//--------------------------------------------------------------------------------------------------
void query_RemoveNumericPushHandler
(
    query_NumericPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    handler_Remove((hub_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'query_StringPush'
 */
//--------------------------------------------------------------------------------------------------
query_StringPushHandlerRef_t query_AddStringPushHandler
(
    const char* path,
        ///< [IN] Absolute path of resource.
    query_StringPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    hub_HandlerRef_t ref = AddPushHandler(path, IO_DATA_TYPE_STRING, callbackPtr, contextPtr);

    return (query_StringPushHandlerRef_t)ref;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'query_StringPush'
 */
//--------------------------------------------------------------------------------------------------
void query_RemoveStringPushHandler
(
    query_StringPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    handler_Remove((hub_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'query_JsonPush'
 */
//--------------------------------------------------------------------------------------------------
query_JsonPushHandlerRef_t query_AddJsonPushHandler
(
    const char* path,
        ///< [IN] Absolute path of resource.
    query_JsonPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    hub_HandlerRef_t ref = AddPushHandler(path, IO_DATA_TYPE_JSON, callbackPtr, contextPtr);

    return (query_JsonPushHandlerRef_t)ref;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'query_JsonPush'
 */
//--------------------------------------------------------------------------------------------------
void query_RemoveJsonPushHandler
(
    query_JsonPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    handler_Remove((hub_HandlerRef_t)handlerRef);
}
