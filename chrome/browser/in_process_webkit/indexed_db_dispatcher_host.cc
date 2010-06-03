// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/in_process_webkit/indexed_db_dispatcher_host.h"

#include "base/command_line.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/in_process_webkit/indexed_db_callbacks.h"
#include "chrome/browser/renderer_host/browser_render_process_host.h"
#include "chrome/browser/renderer_host/resource_message_filter.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/render_messages.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDOMStringList.h"
#include "third_party/WebKit/WebKit/chromium/public/WebIDBDatabase.h"
#include "third_party/WebKit/WebKit/chromium/public/WebIDBDatabaseError.h"
#include "third_party/WebKit/WebKit/chromium/public/WebIDBIndex.h"
#include "third_party/WebKit/WebKit/chromium/public/WebIndexedDatabase.h"
#include "third_party/WebKit/WebKit/chromium/public/WebSecurityOrigin.h"

using WebKit::WebDOMStringList;
using WebKit::WebIDBDatabase;
using WebKit::WebIDBDatabaseError;
using WebKit::WebIDBIndex;
using WebKit::WebSecurityOrigin;

IndexedDBDispatcherHost::IndexedDBDispatcherHost(
    IPC::Message::Sender* sender, WebKitContext* webkit_context)
    : sender_(sender),
      webkit_context_(webkit_context),
      ALLOW_THIS_IN_INITIALIZER_LIST(database_dispatcher_host_(
          new DatabaseDispatcherHost(this))),
      ALLOW_THIS_IN_INITIALIZER_LIST(index_dispatcher_host_(
          new IndexDispatcherHost(this))),
      process_handle_(0) {
  DCHECK(sender_);
  DCHECK(webkit_context_.get());
}

IndexedDBDispatcherHost::~IndexedDBDispatcherHost() {
}

void IndexedDBDispatcherHost::Init(int process_id,
                                   base::ProcessHandle process_handle) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));
  DCHECK(sender_);  // Ensure Shutdown() has not been called.
  DCHECK(!process_handle_);  // Make sure Init() has not yet been called.
  DCHECK(process_handle);
  process_id_ = process_id;
  process_handle_ = process_handle;
}

void IndexedDBDispatcherHost::Shutdown() {
  if (ChromeThread::CurrentlyOn(ChromeThread::IO)) {
    sender_ = NULL;

    bool success = ChromeThread::PostTask(
        ChromeThread::WEBKIT, FROM_HERE,
        NewRunnableMethod(this, &IndexedDBDispatcherHost::Shutdown));
    if (success)
      return;
  }

  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::WEBKIT) ||
         CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess));
  DCHECK(!sender_);

  database_dispatcher_host_.reset();
  index_dispatcher_host_.reset();
}

bool IndexedDBDispatcherHost::OnMessageReceived(const IPC::Message& message) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));
  DCHECK(process_handle_);

  switch (message.type()) {
    case ViewHostMsg_IndexedDatabaseOpen::ID:
    case ViewHostMsg_IDBDatabaseName::ID:
    case ViewHostMsg_IDBDatabaseDescription::ID:
    case ViewHostMsg_IDBDatabaseVersion::ID:
    case ViewHostMsg_IDBDatabaseObjectStores::ID:
    case ViewHostMsg_IDBDatabaseDestroyed::ID:
    case ViewHostMsg_IDBIndexName::ID:
    case ViewHostMsg_IDBIndexKeyPath::ID:
    case ViewHostMsg_IDBIndexUnique::ID:
    case ViewHostMsg_IDBIndexDestroyed::ID:
      break;
    default:
      return false;
  }

  bool success = ChromeThread::PostTask(
      ChromeThread::WEBKIT, FROM_HERE, NewRunnableMethod(
          this, &IndexedDBDispatcherHost::OnMessageReceivedWebKit, message));
  DCHECK(success);
  return true;
}

void IndexedDBDispatcherHost::Send(IPC::Message* message) {
  if (!ChromeThread::CurrentlyOn(ChromeThread::IO)) {
    // TODO(jorlow): Even if we successfully post, I believe it's possible for
    //               the task to never run (if the IO thread is already shutting
    //               down).  We may want to handle this case, though
    //               realistically it probably doesn't matter.
    if (!ChromeThread::PostTask(
            ChromeThread::IO, FROM_HERE, NewRunnableMethod(
                this, &IndexedDBDispatcherHost::Send, message))) {
      // The IO thread is dead.
      delete message;
    }
    return;
  }

  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));
  if (!sender_)
    delete message;
  else
    sender_->Send(message);
}

void IndexedDBDispatcherHost::OnMessageReceivedWebKit(
    const IPC::Message& message) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::WEBKIT));
  DCHECK(process_handle_);

  bool msg_is_ok = true;
  bool handled =
      database_dispatcher_host_->OnMessageReceived(message, &msg_is_ok) ||
      index_dispatcher_host_->OnMessageReceived(message, &msg_is_ok);

  if (!handled) {
    IPC_BEGIN_MESSAGE_MAP_EX(IndexedDBDispatcherHost, message, msg_is_ok)
      IPC_MESSAGE_HANDLER(ViewHostMsg_IndexedDatabaseOpen,
                          OnIndexedDatabaseOpen)
      IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
  }

  DCHECK(handled);
  if (!msg_is_ok) {
    BrowserRenderProcessHost::BadMessageTerminateProcess(message.type(),
                                                         process_handle_);
  }
}

int32 IndexedDBDispatcherHost::AddIDBDatabase(WebIDBDatabase* idb_database) {
  return database_dispatcher_host_->map_.Add(idb_database);
}

class IndexedDatabaseOpenCallbacks : public IndexedDBCallbacks {
 public:
  IndexedDatabaseOpenCallbacks(IndexedDBDispatcherHost* parent,
                               int32 response_id)
      : IndexedDBCallbacks(parent, response_id) {
  }

  virtual void onError(const WebIDBDatabaseError& error) {
    parent()->Send(new ViewMsg_IndexedDatabaseOpenError(
        response_id(), error.code(), error.message()));
  }

  virtual void onSuccess(WebIDBDatabase* idb_database) {
    int32 idb_database_id = parent()->AddIDBDatabase(idb_database);
    parent()->Send(new ViewMsg_IndexedDatabaseOpenSuccess(response_id(),
                                                          idb_database_id));
  }
};

void IndexedDBDispatcherHost::OnIndexedDatabaseOpen(
    const ViewHostMsg_IndexedDatabaseOpen_Params& params) {
  // TODO(jorlow): Check the content settings map and use params.routing_id_
  //               if it's necessary to ask the user for permission.

  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::WEBKIT));
  Context()->GetIndexedDatabase()->open(
      params.name_, params.description_,
      new IndexedDatabaseOpenCallbacks(this, params.response_id_),
      WebSecurityOrigin::createFromDatabaseIdentifier(params.origin_), NULL);
}


//////////////////////////////////////////////////////////////////////
// Helper templates.
//

template <typename ObjectType>
ObjectType* IndexedDBDispatcherHost::GetOrTerminateProcess(
    IDMap<ObjectType, IDMapOwnPointer>* map, int32 return_object_id,
    IPC::Message* reply_msg, uint32 message_type) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::WEBKIT));
  ObjectType* return_object = map->Lookup(return_object_id);
  if (!return_object) {
    BrowserRenderProcessHost::BadMessageTerminateProcess(message_type,
                                                         process_handle_);
    if (reply_msg)
      delete reply_msg;
  }
  return return_object;
}

template <typename ReplyType, typename MessageType,
          typename MapObjectType, typename Method>
void IndexedDBDispatcherHost::SyncGetter(
    IDMap<MapObjectType, IDMapOwnPointer>* map, int32 object_id,
    IPC::Message* reply_msg, Method method) {
  MapObjectType* object = GetOrTerminateProcess(map, object_id, reply_msg,
                                                MessageType::ID);
  if (!object)
      return;

  ReplyType reply = (object->*method)();
  MessageType::WriteReplyParams(reply_msg, reply);
  Send(reply_msg);
}

template <typename ObjectType>
void IndexedDBDispatcherHost::DestroyObject(
    IDMap<ObjectType, IDMapOwnPointer>* map, int32 object_id,
    uint32 message_type) {
  GetOrTerminateProcess(map, object_id, NULL, message_type);
  map->Remove(object_id);
}


//////////////////////////////////////////////////////////////////////
// IndexedDBDispatcherHost::DatabaseDispatcherHost
//

IndexedDBDispatcherHost::DatabaseDispatcherHost::DatabaseDispatcherHost(
    IndexedDBDispatcherHost* parent)
    : parent_(parent) {
}

IndexedDBDispatcherHost::DatabaseDispatcherHost::~DatabaseDispatcherHost() {
}

bool IndexedDBDispatcherHost::DatabaseDispatcherHost::OnMessageReceived(
    const IPC::Message& message, bool* msg_is_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(IndexedDBDispatcherHost::DatabaseDispatcherHost,
                           message, *msg_is_ok)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_IDBDatabaseName, OnName)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_IDBDatabaseDescription,
                                    OnDescription)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_IDBDatabaseVersion, OnVersion)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_IDBDatabaseObjectStores,
                                    OnObjectStores)
    IPC_MESSAGE_HANDLER(ViewHostMsg_IDBDatabaseDestroyed, OnDestroyed)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void IndexedDBDispatcherHost::DatabaseDispatcherHost::Send(
    IPC::Message* message) {
  // The macro magic in OnMessageReceived requires this to link, but it should
  // never actually be called.
  NOTREACHED();
  parent_->Send(message);
}

void IndexedDBDispatcherHost::DatabaseDispatcherHost::OnName(
    int32 object_id, IPC::Message* reply_msg) {
  parent_->SyncGetter<string16, ViewHostMsg_IDBDatabaseName>(
      &map_, object_id, reply_msg, &WebIDBDatabase::name);
}

void IndexedDBDispatcherHost::DatabaseDispatcherHost::OnDescription(
    int32 object_id, IPC::Message* reply_msg) {
  parent_->SyncGetter<string16, ViewHostMsg_IDBDatabaseDescription>(
      &map_, object_id, reply_msg, &WebIDBDatabase::description);
}

void IndexedDBDispatcherHost::DatabaseDispatcherHost::OnVersion(
    int32 object_id, IPC::Message* reply_msg) {
  parent_->SyncGetter<string16, ViewHostMsg_IDBDatabaseVersion>(
      &map_, object_id, reply_msg, &WebIDBDatabase::version);
}

void IndexedDBDispatcherHost::DatabaseDispatcherHost::OnObjectStores(
    int32 idb_database_id, IPC::Message* reply_msg) {
  WebIDBDatabase* idb_database = parent_->GetOrTerminateProcess(
      &map_, idb_database_id, reply_msg,
      ViewHostMsg_IDBDatabaseObjectStores::ID);
  if (!idb_database)
    return;

  WebDOMStringList web_object_stores = idb_database->objectStores();
  std::vector<string16> object_stores;
  for (unsigned i = 0; i < web_object_stores.length(); ++i)
    object_stores[i] = web_object_stores.item(i);
  ViewHostMsg_IDBDatabaseObjectStores::WriteReplyParams(reply_msg,
                                                        object_stores);
  parent_->Send(reply_msg);
}

void IndexedDBDispatcherHost::DatabaseDispatcherHost::OnDestroyed(
    int32 object_id) {
  parent_->DestroyObject(&map_, object_id,
                         ViewHostMsg_IDBDatabaseDestroyed::ID);
}


//////////////////////////////////////////////////////////////////////
// IndexedDBDispatcherHost::IndexDispatcherHost
//

IndexedDBDispatcherHost::IndexDispatcherHost::IndexDispatcherHost(
    IndexedDBDispatcherHost* parent)
    : parent_(parent) {
}

IndexedDBDispatcherHost::IndexDispatcherHost::~IndexDispatcherHost() {
}

bool IndexedDBDispatcherHost::IndexDispatcherHost::OnMessageReceived(
    const IPC::Message& message, bool* msg_is_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(IndexedDBDispatcherHost::IndexDispatcherHost,
                           message, *msg_is_ok)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_IDBIndexName, OnName)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_IDBIndexKeyPath, OnKeyPath)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_IDBIndexUnique, OnUnique)
    IPC_MESSAGE_HANDLER(ViewHostMsg_IDBIndexDestroyed, OnDestroyed)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void IndexedDBDispatcherHost::IndexDispatcherHost::Send(
    IPC::Message* message) {
  // The macro magic in OnMessageReceived requires this to link, but it should
  // never actually be called.
  NOTREACHED();
  parent_->Send(message);
}

void IndexedDBDispatcherHost::IndexDispatcherHost::OnName(
    int32 object_id, IPC::Message* reply_msg) {
  parent_->SyncGetter<string16, ViewHostMsg_IDBIndexName>(
      &map_, object_id, reply_msg, &WebIDBIndex::name);
}

void IndexedDBDispatcherHost::IndexDispatcherHost::OnKeyPath(
    int32 object_id, IPC::Message* reply_msg) {
  parent_->SyncGetter<string16, ViewHostMsg_IDBIndexKeyPath>(
      &map_, object_id, reply_msg, &WebIDBIndex::keyPath);
}

void IndexedDBDispatcherHost::IndexDispatcherHost::OnUnique(
    int32 object_id, IPC::Message* reply_msg) {
  parent_->SyncGetter<bool, ViewHostMsg_IDBIndexUnique>(
      &map_, object_id, reply_msg, &WebIDBIndex::unique);
}

void IndexedDBDispatcherHost::IndexDispatcherHost::OnDestroyed(
    int32 object_id) {
  parent_->DestroyObject(&map_, object_id, ViewHostMsg_IDBIndexDestroyed::ID);
}
