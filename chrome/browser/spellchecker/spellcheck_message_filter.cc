// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spellcheck_message_filter.h"

#include <algorithm>
#include <functional>

#include "base/bind.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#include "chrome/browser/spellchecker/spellcheck_host_metrics.h"
#include "chrome/browser/spellchecker/spellcheck_service.h"
#include "chrome/browser/spellchecker/spelling_service_client.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/spellcheck_marker.h"
#include "chrome/common/spellcheck_messages.h"
#include "content/public/browser/render_process_host.h"
#include "net/url_request/url_fetcher.h"

using content::BrowserThread;

SpellCheckMessageFilter::SpellCheckMessageFilter(int render_process_id)
    : render_process_id_(render_process_id),
      client_(new SpellingServiceClient) {
}

void SpellCheckMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message, BrowserThread::ID* thread) {
  // IPC messages arrive on IO thread, but spellcheck data lives on UI thread.
  // The message filter overrides the thread for these messages because they
  // access spellcheck data.
  if (message.type() == SpellCheckHostMsg_RequestDictionary::ID ||
      message.type() == SpellCheckHostMsg_NotifyChecked::ID ||
      message.type() == SpellCheckHostMsg_RespondDocumentMarkers::ID)
    *thread = BrowserThread::UI;
#if !defined(OS_MACOSX)
  if (message.type() == SpellCheckHostMsg_CallSpellingService::ID)
    *thread = BrowserThread::UI;
#endif
}

bool SpellCheckMessageFilter::OnMessageReceived(const IPC::Message& message,
                                                bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(SpellCheckMessageFilter, message, *message_was_ok)
    IPC_MESSAGE_HANDLER(SpellCheckHostMsg_RequestDictionary,
                        OnSpellCheckerRequestDictionary)
    IPC_MESSAGE_HANDLER(SpellCheckHostMsg_NotifyChecked,
                        OnNotifyChecked)
    IPC_MESSAGE_HANDLER(SpellCheckHostMsg_RespondDocumentMarkers,
                        OnRespondDocumentMarkers)
#if !defined(OS_MACOSX)
    IPC_MESSAGE_HANDLER(SpellCheckHostMsg_CallSpellingService,
                        OnCallSpellingService)
#endif
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

SpellCheckMessageFilter::~SpellCheckMessageFilter() {}

void SpellCheckMessageFilter::OnSpellCheckerRequestDictionary() {
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(render_process_id_);
  if (!host)
    return;  // Teardown.
  Profile* profile = Profile::FromBrowserContext(host->GetBrowserContext());
  // The renderer has requested that we initialize its spellchecker. This should
  // generally only be called once per session, as after the first call, all
  // future renderers will be passed the initialization information on startup
  // (or when the dictionary changes in some way).
  SpellcheckService* spellcheck_service =
      SpellcheckServiceFactory::GetForProfile(profile);

  DCHECK(spellcheck_service);
  // The spellchecker initialization already started and finished; just send
  // it to the renderer.
  spellcheck_service->InitForRenderer(host);

  // TODO(rlp): Ensure that we do not initialize the hunspell dictionary more
  // than once if we get requests from different renderers.
}

void SpellCheckMessageFilter::OnNotifyChecked(const string16& word,
                                              bool misspelled) {
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(render_process_id_);
  if (!host)
    return;  // Teardown.
  // Delegates to SpellCheckHost which tracks the stats of our spellchecker.
  Profile* profile = Profile::FromBrowserContext(host->GetBrowserContext());
  SpellcheckService* spellcheck_service =
      SpellcheckServiceFactory::GetForProfile(profile);
  if (!spellcheck_service)
    return;
  if (spellcheck_service->GetMetrics())
    spellcheck_service->GetMetrics()->RecordCheckedWordStats(word, misspelled);
}

void SpellCheckMessageFilter::OnRespondDocumentMarkers(
    const std::vector<uint32>& markers) {
  SpellcheckService* spellcheck =
      SpellcheckServiceFactory::GetForRenderProcessId(render_process_id_);
  // Spellcheck service may not be available for a renderer process that is
  // shutting down.
  if (!spellcheck)
    return;
  spellcheck->GetFeedbackSender()->OnReceiveDocumentMarkers(
      render_process_id_, markers);
}

#if !defined(OS_MACOSX)
void SpellCheckMessageFilter::OnCallSpellingService(
    int route_id,
    int identifier,
    const string16& text,
    std::vector<SpellCheckMarker> markers) {
  DCHECK(!text.empty());
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  // Erase invalid markers (with offsets out of boundaries of text length).
  markers.erase(
      std::remove_if(
          markers.begin(),
          markers.end(),
          std::not1(SpellCheckMarker::IsValidPredicate(text.length()))),
      markers.end());
  CallSpellingService(text, route_id, identifier, markers);
}

void SpellCheckMessageFilter::OnTextCheckComplete(
    int route_id,
    int identifier,
    const std::vector<SpellCheckMarker>& markers,
    bool success,
    const string16& text,
    const std::vector<SpellCheckResult>& results) {
  SpellcheckService* spellcheck =
      SpellcheckServiceFactory::GetForRenderProcessId(render_process_id_);
  if (!spellcheck)
    return;
  std::vector<SpellCheckResult> results_copy = results;
  spellcheck->GetFeedbackSender()->OnSpellcheckResults(
      &results_copy, render_process_id_, text, markers);
  Send(new SpellCheckMsg_RespondSpellingService(
      route_id, identifier, success, text, results_copy));
}

// CallSpellingService always executes the callback OnTextCheckComplete.
// (Which, in turn, sends a SpellCheckMsg_RespondSpellingService)
void SpellCheckMessageFilter::CallSpellingService(
    const string16& text,
    int route_id,
    int identifier,
    const std::vector<SpellCheckMarker>& markers) {
  Profile* profile = NULL;
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(render_process_id_);
  if (host)
    profile = Profile::FromBrowserContext(host->GetBrowserContext());

  client_->RequestTextCheck(
    profile,
    SpellingServiceClient::SPELLCHECK,
    text,
    base::Bind(&SpellCheckMessageFilter::OnTextCheckComplete,
               base::Unretained(this),
               route_id,
               identifier,
               markers));
}
#endif
