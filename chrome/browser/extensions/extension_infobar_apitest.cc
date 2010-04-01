// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_switches.h"

#if defined(TOOLKIT_VIEWS)
#if defined (OS_WIN)
#define MAYBE_Infobars Infobars
#else
// Flaky on ChromeOS, see http://crbug.com/40141.
#define MAYBE_Infobars FLAKY_Infobars
#endif

#else
// Need to port ExtensionInfoBarDelegate::CreateInfoBar() to other platforms.
// See http://crbug.com/39916 for details.
#define MAYBE_Infobars DISABLED_Infobars
#endif

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_Infobars) {
  // TODO(finnur): Remove once infobars are no longer experimental (bug 39511).
  CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableExperimentalExtensionApis);

  ASSERT_TRUE(RunExtensionTest("infobars")) << message_;
}
