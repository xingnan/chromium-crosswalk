// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_POLICY_LOADER_WIN_H_
#define CHROME_BROWSER_POLICY_POLICY_LOADER_WIN_H_

#include <userenv.h>
#include <windows.h>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/values.h"
#include "base/win/object_watcher.h"
#include "chrome/browser/policy/async_policy_loader.h"
#include "chrome/browser/policy/policy_types.h"

namespace policy {

class AppliedGPOListProvider;
class PolicyLoadStatusSample;
class PolicyMap;
struct PolicyDefinitionList;

// Interface for mocking out GPO enumeration in tests.
class AppliedGPOListProvider {
 public:
  virtual ~AppliedGPOListProvider() {}
  virtual DWORD GetAppliedGPOList(DWORD flags,
                                  LPCTSTR machine_name,
                                  PSID sid_user,
                                  GUID* extension_guid,
                                  PGROUP_POLICY_OBJECT* gpo_list) = 0;
  virtual BOOL FreeGPOList(PGROUP_POLICY_OBJECT gpo_list) = 0;
};

// Loads policies from the Windows registry, and watches for Group Policy
// notifications to trigger reloads.
class PolicyLoaderWin : public AsyncPolicyLoader,
                        public base::win::ObjectWatcher::Delegate {
 public:
  // The PReg file name used by GPO.
  static const base::FilePath::CharType kPRegFileName[];

  explicit PolicyLoaderWin(const PolicyDefinitionList* policy_list,
                           const string16& chrome_policy_key,
                           AppliedGPOListProvider* gpo_provider);
  virtual ~PolicyLoaderWin();

  // Creates a policy loader that uses the Win API to access GPO.
  static scoped_ptr<PolicyLoaderWin> Create(
      const PolicyDefinitionList* policy_list);

  // AsyncPolicyLoader implementation.
  virtual void InitOnFile() OVERRIDE;
  virtual scoped_ptr<PolicyBundle> Load() OVERRIDE;

 private:
  // Builds the Chrome policy schema in |chrome_policy_schema_|.
  void BuildChromePolicySchema();

  // Reads Chrome Policy from a PReg file at the given path and stores the
  // result in |policy|.
  bool ReadPRegFile(const base::FilePath& preg_file,
                    base::DictionaryValue* policy,
                    PolicyLoadStatusSample *status);

  // Loads and parses GPO policy in |policy_object_list| for scope |scope|. If
  // successful, stores the result in |policy| and returns true. Returns false
  // on failure reading the policy, indicating that policy loading should fall
  // back to reading the registry.
  bool LoadGPOPolicy(PolicyScope scope,
                     PGROUP_POLICY_OBJECT policy_object_list,
                     base::DictionaryValue* policy,
                     PolicyLoadStatusSample *status);

  // Queries Windows for applied group policy and writes the result to |policy|.
  // This is the preferred way to obtain GPO data, there are reports of abuse
  // of the registry GPO keys by 3rd-party software.
  bool ReadPolicyFromGPO(PolicyScope scope,
                         base::DictionaryValue* policy,
                         PolicyLoadStatusSample *status);

  // Parses Chrome policy from |gpo_dict| for the given |scope| and |level| and
  // merges it into |chrome_policy_map|.
  void LoadChromePolicy(const base::DictionaryValue* gpo_dict,
                        PolicyLevel level,
                        PolicyScope scope,
                        PolicyMap* chrome_policy_map);

  // Loads 3rd-party policy from |gpo_dict| and merges it into |bundle|.
  void Load3rdPartyPolicy(const base::DictionaryValue* gpo_dict,
                          PolicyScope scope,
                          PolicyBundle* bundle);

  // Installs the watchers for the Group Policy update events.
  void SetupWatches();

  // ObjectWatcher::Delegate overrides:
  virtual void OnObjectSignaled(HANDLE object) OVERRIDE;

  bool is_initialized_;
  const PolicyDefinitionList* policy_list_;
  const string16 chrome_policy_key_;
  class AppliedGPOListProvider* gpo_provider_;
  base::DictionaryValue chrome_policy_schema_;

  base::WaitableEvent user_policy_changed_event_;
  base::WaitableEvent machine_policy_changed_event_;
  base::win::ObjectWatcher user_policy_watcher_;
  base::win::ObjectWatcher machine_policy_watcher_;
  bool user_policy_watcher_failed_;
  bool machine_policy_watcher_failed_;

  DISALLOW_COPY_AND_ASSIGN(PolicyLoaderWin);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_POLICY_LOADER_WIN_H_
