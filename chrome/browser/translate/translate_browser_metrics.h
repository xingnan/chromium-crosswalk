// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TRANSLATE_TRANSLATE_BROWSER_METRICS_H_
#define CHROME_BROWSER_TRANSLATE_TRANSLATE_BROWSER_METRICS_H_

#include <string>

namespace TranslateBrowserMetrics {

// An indexing type to query each UMA entry name via GetMetricsName() function.
// Note: |kMetricsEntries| should be updated when a new entry is added here.
enum MetricsNameIndex {
  UMA_INITIATION_STATUS,
  UMA_LANGUAGE_DETECTION_ERROR,
  UMA_LOCALES_ON_DISABLED_BY_PREFS,
  UMA_UNDISPLAYABLE_LANGUAGE,
  UMA_UNSUPPORTED_LANGUAGE_AT_INITIATION,
  UMA_MAX,
};

// When Chrome Translate is ready to translate a page, one of following reason
// decide the next browser action.
enum InitiationStatusType {
  INITIATION_STATUS_DISABLED_BY_PREFS,
  INITIATION_STATUS_DISABLED_BY_SWITCH,
  INITIATION_STATUS_DISABLED_BY_CONFIG,
  INITIATION_STATUS_LANGUAGE_IS_NOT_SUPPORTED,
  INITIATION_STATUS_URL_IS_NOT_SUPPORTED,
  INITIATION_STATUS_SIMILAR_LANGUAGES,
  INITIATION_STATUS_ACCEPT_LANGUAGES,

  INITIATION_STATUS_AUTO_BY_CONFIG,
  INITIATION_STATUS_AUTO_BY_LINK,
  INITIATION_STATUS_SHOW_INFOBAR,

  INITIATION_STATUS_MAX,
};

// Called when Chrome Translate is initiated to report a reason of the next
// browser action.
void ReportInitiationStatus(InitiationStatusType type);

// Called when Chrome opens the URL so that the user sends an error feedback.
void ReportLanguageDetectionError();

void ReportLocalesOnDisabledByPrefs(const std::string& locale);

// Called when Chrome Translate server sends the language list which includes
// a undisplayable language in the user's locale.
void ReportUndisplayableLanguage(const std::string& language);

void ReportUnsupportedLanguageAtInitiation(const std::string& language);

// Provides UMA entry names for unit tests.
const char* GetMetricsName(MetricsNameIndex index);

}  // namespace TranslateBrowserMetrics

#endif  // CHROME_BROWSER_TRANSLATE_TRANSLATE_BROWSER_METRICS_H_
