//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the I/O API service interfaces served up by the Data Hub.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "dataHub.h"


//--------------------------------------------------------------------------------------------------
/**
 * Get the client app's namespace.
 *
 * @return the reference to the namespace resource tree entry or NULL if failed.
 */
//--------------------------------------------------------------------------------------------------
static resTree_EntryRef_t GetClientNamespace
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // See if we already looked up the app name, etc. and saved it as the IPC session context ptr.
    le_msg_SessionRef_t sessionRef = io_GetClientSessionRef();
    resTree_EntryRef_t nsRef = le_msg_GetSessionContextPtr(sessionRef);
    if (nsRef != NULL)
    {
        return nsRef;
    }

    // Get the client app name.
    pid_t clientPid;
    le_result_t result = le_msg_GetClientProcessId(sessionRef, &clientPid);
    if (result != LE_OK)
    {
        LE_KILL_CLIENT("Unable to retrieve client process ID (%s)", LE_RESULT_TXT(result));
        return NULL;
    }
    char appName[LE_LIMIT_APP_NAME_LEN + 1];
    result = le_appInfo_GetName(clientPid, appName, sizeof(appName));
    if (result != LE_OK)
    {
        LE_KILL_CLIENT("Unable to retrieve client app name (%s)", LE_RESULT_TXT(result));
        return NULL;
    }

    // Get the "/app" namespace first.
    nsRef = resTree_GetEntry(resTree_GetRoot(), "app");

    // Now get the app's namespace under the /app namespace.
    nsRef = resTree_GetEntry(nsRef, appName);

    // Store the namespace entry reference as the IPC session Context Ptr to speed things up
    // next time.
    le_msg_SetSessionContextPtr(sessionRef, nsRef);

    return nsRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the resource at a given path within the app's namespace.
 *
 * @return Reference to the entry, or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
static resTree_EntryRef_t FindResource
(
    const char* path  ///< Resource path within the client app's namespace.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t entryRef = GetClientNamespace();
    if (entryRef != NULL)
    {
        entryRef = resTree_FindEntry(entryRef, path);
    }

    admin_EntryType_t entryType = resTree_GetEntryType(entryRef);
    if ((entryType != ADMIN_ENTRY_TYPE_INPUT) && (entryType != ADMIN_ENTRY_TYPE_OUTPUT))
    {
        LE_DEBUG("'%s' is not an Input or an Output.", path);
        return NULL;
    }

    return entryRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create an input resource, which is used to push data into the Data Hub.
 *
 * Does nothing if the resource already exists.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_DUPLICATE if a resource by that name exists but with different direction, type or units.
 *  - LE_NO_MEMORY if the client is not permitted to create that many resources.
 */
//--------------------------------------------------------------------------------------------------
le_result_t io_CreateInput
(
    const char* path,
        ///< [IN] Path within the client app's resource namespace.
    io_DataType_t dataType,
        ///< [IN] The data type.
    const char* units
        ///< [IN] e.g., "degC" (see senml); "" = unspecified.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = NULL;

    resTree_EntryRef_t nsRef = GetClientNamespace();

    // Check for another resource having the same name in the same namespace.
    if (nsRef != NULL)
    {
        resRef = resTree_FindEntry(nsRef, path);
    }
    if (resRef != NULL)
    {
        switch (resTree_GetEntryType(resRef))
        {
            case ADMIN_ENTRY_TYPE_INPUT:

                // Check data type and units to see if they match.
                if (   (resTree_GetDataType(resRef) != dataType)
                    || (strcmp(units, resTree_GetUnits(resRef)) != 0))
                {
                    return LE_DUPLICATE;
                }

                // The object already exists.  Nothing more needs to be done.
                return LE_OK;

            case ADMIN_ENTRY_TYPE_OUTPUT:
            case ADMIN_ENTRY_TYPE_OBSERVATION:

                // These conflict.
                return LE_DUPLICATE;

            case ADMIN_ENTRY_TYPE_NAMESPACE:
            case ADMIN_ENTRY_TYPE_PLACEHOLDER:

                // These can be upgraded to Input objects by resTree_GetInput().
                break;

            case ADMIN_ENTRY_TYPE_NONE:

                LE_FATAL("Unexpected entry type.");
        }
    }

    // Get/Create the Input resource.
    resRef = resTree_GetInput(nsRef, path, dataType, units);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Failed to create Input '/app/%s/%s'.", resTree_GetEntryName(nsRef), path);
        return LE_FAULT;    // Client has been killed, so it doesn't matter what we return.
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create an output resource, which is used to push data into the Data Hub.
 *
 * Does nothing if the resource already exists.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_DUPLICATE if a resource by that name exists but with different direction, type or units.
 *  - LE_NO_MEMORY if the client is not permitted to create that many resources.
 */
//--------------------------------------------------------------------------------------------------
le_result_t io_CreateOutput
(
    const char* path,
        ///< [IN] Path within the client app's resource namespace.
    io_DataType_t dataType,
        ///< [IN] The data type.
    const char* units
        ///< [IN] e.g., "degC" (see senml); "" = unspecified.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = NULL;

    resTree_EntryRef_t nsRef = GetClientNamespace();

    // Check for another resource having the same name in the same namespace.
    if (nsRef != NULL)
    {
        resRef = resTree_FindEntry(nsRef, path);
    }
    if (resRef != NULL)
    {
        switch (resTree_GetEntryType(resRef))
        {
            case ADMIN_ENTRY_TYPE_OUTPUT:

                // Check data type and units to see if they match.
                if (   (resTree_GetDataType(resRef) != dataType)
                    || (strcmp(units, resTree_GetUnits(resRef)) != 0))
                {
                    return LE_DUPLICATE;
                }

                // The object already exists.  Nothing more needs to be done.
                return LE_OK;

            case ADMIN_ENTRY_TYPE_INPUT:
            case ADMIN_ENTRY_TYPE_OBSERVATION:

                // These conflict.
                return LE_DUPLICATE;

            case ADMIN_ENTRY_TYPE_NAMESPACE:
            case ADMIN_ENTRY_TYPE_PLACEHOLDER:

                // These can be upgraded to Output objects by resTree_GetOutput().
                break;

            case ADMIN_ENTRY_TYPE_NONE:

                LE_FATAL("Unexpected entry type.");
        }
    }

    // Create the Output resource.
    resRef = resTree_GetOutput(nsRef, path, dataType, units);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Failed to create Output '/app/%s/%s'.", resTree_GetEntryName(nsRef), path);
        return LE_FAULT;    // Client has been killed, so it doesn't matter what we return.
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a resource.
 *
 * Does nothing if the resource doesn't exist.
 */
//--------------------------------------------------------------------------------------------------
void io_DeleteResource
(
    const char* path
        ///< [IN] Resource path within the client app's namespace.
)
//--------------------------------------------------------------------------------------------------
{
    // If the resource exists, delete it.
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef != NULL)
    {
        resTree_DeleteIO(resRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Push a trigger type data sample.
 */
//--------------------------------------------------------------------------------------------------
void io_PushTrigger
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double timestamp
        ///< [IN] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
        ///< Zero = now.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Client tried to push data to a non-existent resource '%s'.", path);
        return;
    }

    // Create a Data Sample object for this new sample.
    dataSample_Ref_t sampleRef = dataSample_CreateTrigger(timestamp);

    // Push the sample to the Resource.
    resTree_Push(resRef, IO_DATA_TYPE_TRIGGER, sampleRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Push a Boolean type data sample.
 */
//--------------------------------------------------------------------------------------------------
void io_PushBoolean
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double timestamp,
        ///< [IN] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
        ///< Zero = now.
    bool value
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Client tried to push data to a non-existent resource '%s'.", path);
        return;
    }

    // Create a Data Sample object for this new sample.
    dataSample_Ref_t sampleRef = dataSample_CreateBoolean(timestamp, value);

    // Push the sample to the Resource.
    resTree_Push(resRef, IO_DATA_TYPE_BOOLEAN, sampleRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Push a numeric type data sample.
 */
//--------------------------------------------------------------------------------------------------
void io_PushNumeric
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double timestamp,
        ///< [IN] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
        ///< Zero = now.
    double value
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Client tried to push data to a non-existent resource '%s'.", path);
        return;
    }

    // Create a Data Sample object for this new sample.
    dataSample_Ref_t sampleRef = dataSample_CreateNumeric(timestamp, value);

    // Push the sample to the Resource.
    resTree_Push(resRef, IO_DATA_TYPE_NUMERIC, sampleRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Push a string type data sample.
 */
//--------------------------------------------------------------------------------------------------
void io_PushString
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double timestamp,
        ///< [IN] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
        ///< Zero = now.
    const char* value
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Client tried to push data to a non-existent resource '%s'.", path);
        return;
    }

    // Create a Data Sample object for this new sample.
    dataSample_Ref_t sampleRef = dataSample_CreateString(timestamp, value);

    // Push the sample to the Resource.
    resTree_Push(resRef, IO_DATA_TYPE_STRING, sampleRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Push a JSON data sample.
 */
//--------------------------------------------------------------------------------------------------
void io_PushJson
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double timestamp,
        ///< [IN] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
        ///< Zero = now.
    const char* value
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Client tried to push data to a non-existent resource '%s'.", path);
        return;
    }

    // Create a Data Sample object for this new sample.
    dataSample_Ref_t sampleRef = dataSample_CreateJson(timestamp, value);

    // Push the sample to the Resource.
    resTree_Push(resRef, IO_DATA_TYPE_JSON, sampleRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for an event.
 *
 * @return A reference to the handler or NULL if failed and client has been killed.
 */
//--------------------------------------------------------------------------------------------------
static hub_HandlerRef_t AddPushHandler
(
    const char* path,   ///< Resource path within the client app's namespace.
    io_DataType_t dataType,
    void* callbackPtr,  ///< Callback function pointer
    void* contextPtr
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t nsRef = GetClientNamespace();
    if (nsRef == NULL)
    {
        LE_KILL_CLIENT("Client tried to register a push handler before creating any resources.");
        return NULL;
    }

    resTree_EntryRef_t resRef = resTree_FindEntry(nsRef, path);

    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Attempt to register Push handler on non-existent resource '/app/%s/%s'.",
                       resTree_GetEntryName(nsRef),
                       path);
        return NULL;
    }

    admin_EntryType_t entryType = resTree_GetEntryType(resRef);
    if ((entryType != ADMIN_ENTRY_TYPE_INPUT) && (entryType != ADMIN_ENTRY_TYPE_OUTPUT))
    {
        LE_KILL_CLIENT("Attempt to register Push handler before creating resource '/app/%s/%s'.",
                       resTree_GetEntryName(nsRef),
                       path);
        return NULL;
    }

    return resTree_AddPushHandler(resRef, dataType, callbackPtr, contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'io_TriggerPush'
 */
//--------------------------------------------------------------------------------------------------
io_TriggerPushHandlerRef_t io_AddTriggerPushHandler
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    io_TriggerPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    return (io_TriggerPushHandlerRef_t)AddPushHandler(path,
                                                      IO_DATA_TYPE_TRIGGER,
                                                      callbackPtr,
                                                      contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'io_TriggerPush'
 */
//--------------------------------------------------------------------------------------------------
void io_RemoveTriggerPushHandler
(
    io_TriggerPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_RemovePushHandler((hub_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'io_BooleanPush'
 */
//--------------------------------------------------------------------------------------------------
io_BooleanPushHandlerRef_t io_AddBooleanPushHandler
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    io_BooleanPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    return (io_BooleanPushHandlerRef_t)AddPushHandler(path,
                                                      IO_DATA_TYPE_BOOLEAN,
                                                      callbackPtr,
                                                      contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'io_BooleanPush'
 */
//--------------------------------------------------------------------------------------------------
void io_RemoveBooleanPushHandler
(
    io_BooleanPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_RemovePushHandler((hub_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'io_NumericPush'
 */
//--------------------------------------------------------------------------------------------------
io_NumericPushHandlerRef_t io_AddNumericPushHandler
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    io_NumericPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    return (io_NumericPushHandlerRef_t)AddPushHandler(path,
                                                      IO_DATA_TYPE_NUMERIC,
                                                      callbackPtr,
                                                      contextPtr);
}



//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'io_NumericPush'
 */
//--------------------------------------------------------------------------------------------------
void io_RemoveNumericPushHandler
(
    io_NumericPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_RemovePushHandler((hub_HandlerRef_t)handlerRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'io_StringPush'
 */
//--------------------------------------------------------------------------------------------------
io_StringPushHandlerRef_t io_AddStringPushHandler
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    io_StringPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    return (io_StringPushHandlerRef_t)AddPushHandler(path,
                                                     IO_DATA_TYPE_STRING,
                                                     callbackPtr,
                                                     contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'io_StringPush'
 */
//--------------------------------------------------------------------------------------------------
void io_RemoveStringPushHandler
(
    io_StringPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_RemovePushHandler((hub_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'io_JsonPush'
 */
//--------------------------------------------------------------------------------------------------
io_JsonPushHandlerRef_t io_AddJsonPushHandler
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    io_JsonPushHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    return (io_JsonPushHandlerRef_t)AddPushHandler(path,
                                                   IO_DATA_TYPE_JSON,
                                                   callbackPtr,
                                                   contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'io_JsonPush'
 */
//--------------------------------------------------------------------------------------------------
void io_RemoveJsonPushHandler
(
    io_JsonPushHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_RemovePushHandler((hub_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark an Output resource "optional".  (By default, they are marked "mandatory".)
 */
//--------------------------------------------------------------------------------------------------
void io_MarkOptional
(
    const char* path
        ///< [IN] Resource path within the client app's namespace.
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Attempt to mark non-existent resource optional at '%s'.", path);
    }
    else if (resTree_GetEntryType(resRef) != ADMIN_ENTRY_TYPE_OUTPUT)
    {
        LE_KILL_CLIENT("Attempt to mark non-Output resource optional at '%s'.", path);
    }
    else
    {
        resTree_MarkOptional(resRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set a Boolean type value as the default value of a given resource.
 *
 * @note This will be ignored if the resource already has a default value.
 */
//--------------------------------------------------------------------------------------------------
void io_SetBooleanDefault
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    bool value
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Attempt to set default value of non-existent resource '%s'.", path);
    }
    else if (resTree_GetDataType(resRef) != IO_DATA_TYPE_BOOLEAN)
    {
        LE_KILL_CLIENT("Attempt to set default value to wrong type for resource '%s'.", path);
    }
    else if (!resTree_HasDefault(resRef))
    {
        // Create a Data Sample object for this new sample.
        dataSample_Ref_t sampleRef = dataSample_CreateBoolean(0.0, value);

        resTree_SetDefault(resRef, IO_DATA_TYPE_BOOLEAN, sampleRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set a numeric type value as the default value of a given resource.
 *
 * @note This will be ignored if the resource already has a default value.
 */
//--------------------------------------------------------------------------------------------------
void io_SetNumericDefault
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double value
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Attempt to set default value of non-existent resource '%s'.", path);
    }
    else if (resTree_GetDataType(resRef) != IO_DATA_TYPE_NUMERIC)
    {
        LE_KILL_CLIENT("Attempt to set default value to wrong type for resource '%s'.", path);
    }
    else if (!resTree_HasDefault(resRef))
    {
        // Create a Data Sample object for this new sample.
        dataSample_Ref_t sampleRef = dataSample_CreateNumeric(0.0, value);

        resTree_SetDefault(resRef, IO_DATA_TYPE_NUMERIC, sampleRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set a string type value as the default value of a given resource.
 *
 * @note This will be ignored if the resource already has a default value.
 */
//--------------------------------------------------------------------------------------------------
void io_SetStringDefault
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    const char* value
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Attempt to set default value of non-existent resource '%s'.", path);
    }
    else if (resTree_GetDataType(resRef) != IO_DATA_TYPE_STRING)
    {
        LE_KILL_CLIENT("Attempt to set default value to wrong type for resource '%s'.", path);
    }
    else if (!resTree_HasDefault(resRef))
    {
        // Create a Data Sample object for this new sample.
        dataSample_Ref_t sampleRef = dataSample_CreateString(0.0, value);

        resTree_SetDefault(resRef, IO_DATA_TYPE_STRING, sampleRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set a JSON type value as the default value of a given resource.
 *
 * @note This will be ignored if the resource already has a default value.
 */
//--------------------------------------------------------------------------------------------------
void io_SetJsonDefault
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    const char* value
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        LE_KILL_CLIENT("Attempt to set default value of non-existent resource '%s'.", path);
    }
    else if (resTree_GetDataType(resRef) != IO_DATA_TYPE_JSON)
    {
        LE_KILL_CLIENT("Attempt to set default value to wrong type for resource '%s'.", path);
    }
    else if (!resTree_HasDefault(resRef))
    {
        // Create a Data Sample object for this new sample.
        dataSample_Ref_t sampleRef = dataSample_CreateJson(0.0, value);

        resTree_SetDefault(resRef, IO_DATA_TYPE_JSON, sampleRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the current value of a given resource, with type check.
 *
 * @return A reference to the data sample, or NULL if not available.
 *
 * @note Kills the client if the type doesn't match.
 */
//--------------------------------------------------------------------------------------------------
static dataSample_Ref_t GetCurrentValue
(
    resTree_EntryRef_t resRef,
    io_DataType_t dataType
)
//--------------------------------------------------------------------------------------------------
{
    if (resTree_GetDataType(resRef) != dataType)
    {
        LE_KILL_CLIENT("Fetch of wrong data type on resource.");
        return NULL;
    }

    return resTree_GetCurrentValue(resRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetch the timestamp of the current value of an Input or Output resource with any data type.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource does not exist.
 *  - LE_UNAVAILABLE if the resource does not currently have a value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t io_GetTimestamp
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double* timestampPtr
        ///< [OUT] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    dataSample_Ref_t currentValue = resTree_GetCurrentValue(resRef);
    if (currentValue == NULL)
    {
        return LE_UNAVAILABLE;
    }

    *timestampPtr = dataSample_GetTimestamp(currentValue);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetch the current value of a Boolean type Input or Output resource.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource does not exist.
 *  - LE_UNAVAILABLE if the resource does not currently have a value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t io_GetBoolean
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double* timestampPtr,
        ///< [OUT] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
    bool* valuePtr
        ///< [OUT]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    dataSample_Ref_t currentValue = GetCurrentValue(resRef, IO_DATA_TYPE_BOOLEAN);
    if (currentValue == NULL)
    {
        return LE_UNAVAILABLE;
    }

    *valuePtr = dataSample_GetBoolean(currentValue);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetch the current value of a numeric type Input or Output resource.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the resource does not exist.
 *  - LE_UNAVAILABLE if the resource does not currently have a value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t io_GetNumeric
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double* timestampPtr,
        ///< [OUT] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
    double* valuePtr
        ///< [OUT]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    dataSample_Ref_t currentValue = GetCurrentValue(resRef, IO_DATA_TYPE_NUMERIC);
    if (currentValue == NULL)
    {
        return LE_UNAVAILABLE;
    }

    *valuePtr = dataSample_GetNumeric(currentValue);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetch the current value of a string type Input or Output resource.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_OVERFLOW if the value buffer was too small to hold the value.
 *  - LE_NOT_FOUND if the resource does not exist.
 *  - LE_UNAVAILABLE if the resource does not currently have a value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t io_GetString
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double* timestampPtr,
        ///< [OUT] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
    char* value,
        ///< [OUT]
    size_t valueSize
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    dataSample_Ref_t currentValue = GetCurrentValue(resRef, IO_DATA_TYPE_STRING);
    if (currentValue == NULL)
    {
        return LE_UNAVAILABLE;
    }

    return le_utf8_Copy(value, dataSample_GetString(currentValue), valueSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetch the current value of an Input or Output resource (of any data type) in JSON format.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_OVERFLOW if the value buffer was too small to hold the value.
 *  - LE_NOT_FOUND if the resource does not exist.
 *  - LE_UNAVAILABLE if the resource does not currently have a value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t io_GetJson
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    double* timestampPtr,
        ///< [OUT] Timestamp in seconds since the Epoch 1970-01-01 00:00:00 +0000 (UTC).
    char* value,
        ///< [OUT]
    size_t valueSize
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    resTree_EntryRef_t resRef = FindResource(path);
    if (resRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    dataSample_Ref_t currentValue = resTree_GetCurrentValue(resRef);
    if (currentValue == NULL)
    {
        return LE_UNAVAILABLE;
    }

    return dataSample_ConvertToJson(currentValue, resTree_GetDataType(resRef), value, valueSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'io_Poll'
 */
//--------------------------------------------------------------------------------------------------
io_PollHandlerRef_t io_AddPollHandler
(
    const char* path,
        ///< [IN] Resource path within the client app's namespace.
    io_PollHandlerFunc_t callbackPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
    LE_WARN("Polling not yet supported.");

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'io_Poll'
 */
//--------------------------------------------------------------------------------------------------
void io_RemovePollHandler
(
    io_PollHandlerRef_t handlerRef
        ///< [IN]
)
//--------------------------------------------------------------------------------------------------
{
}
