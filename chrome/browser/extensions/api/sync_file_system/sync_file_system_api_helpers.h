// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SYNC_FILE_SYSTEM_SYNC_FILE_SYSTEM_API_HELPERS_H_
#define CHROME_BROWSER_EXTENSIONS_API_SYNC_FILE_SYSTEM_SYNC_FILE_SYSTEM_API_HELPERS_H_

#include "chrome/browser/sync_file_system/conflict_resolution_policy.h"
#include "chrome/browser/sync_file_system/sync_service_state.h"
#include "chrome/common/extensions/api/sync_file_system.h"
#include "webkit/browser/fileapi/syncable/sync_action.h"
#include "webkit/browser/fileapi/syncable/sync_direction.h"
#include "webkit/browser/fileapi/syncable/sync_file_status.h"
#include "webkit/browser/fileapi/syncable/sync_file_type.h"

namespace fileapi {
class FileSystemURL;
}

namespace base {
class DictionaryValue;
}

namespace extensions {

// extensions::api::sync_file_system <-> sync_file_system enum conversion
// functions.
api::sync_file_system::ServiceStatus SyncServiceStateToExtensionEnum(
    sync_file_system::SyncServiceState state);

api::sync_file_system::FileStatus SyncFileStatusToExtensionEnum(
    sync_file_system::SyncFileStatus status);

api::sync_file_system::SyncAction SyncActionToExtensionEnum(
    sync_file_system::SyncAction action);

api::sync_file_system::SyncDirection SyncDirectionToExtensionEnum(
    sync_file_system::SyncDirection direction);

api::sync_file_system::ConflictResolutionPolicy
ConflictResolutionPolicyToExtensionEnum(
    sync_file_system::ConflictResolutionPolicy policy);

sync_file_system::ConflictResolutionPolicy
ExtensionEnumToConflictResolutionPolicy(
    api::sync_file_system::ConflictResolutionPolicy);

// Creates a dictionary for FileSystem Entry from given |url|.
// This will create a dictionary which has 'fileSystemType', 'fileSystemName',
// 'rootUrl', 'filePath' and 'isDirectory' fields.
// The returned dictionary is supposed to be interpreted
// in the renderer's customer binding to create a FileEntry object.
// This returns NULL if the given |url| is not valid or |file_type| is
// SYNC_FILE_TYPE_UNKNOWN.
base::DictionaryValue* CreateDictionaryValueForFileSystemEntry(
    const fileapi::FileSystemURL& url,
    sync_file_system::SyncFileType file_type);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SYNC_FILE_SYSTEM_SYNC_FILE_SYSTEM_API_HELPERS_H_
