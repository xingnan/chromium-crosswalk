// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SMS_CLIENT_H_
#define CHROMEOS_DBUS_SMS_CLIENT_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client_implementation_type.h"

namespace base {
class DictionaryValue;
}

namespace dbus {
class Bus;
class ObjectPath;
}

namespace chromeos {

// SMSMessageClient is used to communicate with the
// org.freedesktop.ModemManager1.SMS service.  All methods should be
// called from the origin thread (UI thread) which initializes the
// DBusThreadManager instance.
class CHROMEOS_EXPORT SMSClient {
 public:
  typedef base::Callback<void(const base::DictionaryValue& sms)> GetAllCallback;

  virtual ~SMSClient();

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static SMSClient* Create(DBusClientImplementationType type,
                           dbus::Bus* bus);

  // Calls GetAll method.  |callback| is called after the method call succeeds.
  virtual void GetAll(const std::string& service_name,
                      const dbus::ObjectPath& object_path,
                      const GetAllCallback& callback) = 0;

 protected:
  // Create() should be used instead.
  SMSClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(SMSClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SMS_CLIENT_H_
