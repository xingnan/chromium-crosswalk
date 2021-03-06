// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/default_app_order.h"

#include <string>
#include <vector>

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_ptr.h"
#include "base/path_service.h"
#include "base/test/scoped_path_override.h"
#include "chromeos/chromeos_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace {

const base::FilePath::CharType kTestFile[] =
    FILE_PATH_LITERAL("test_default_app_order.json");
}

class DefaultAppOrderTest : public testing::Test {
 public:
  DefaultAppOrderTest() {}
  virtual ~DefaultAppOrderTest() {}

  // testing::Test overrides:
  virtual void SetUp() OVERRIDE {
    default_app_order::Get(&built_in_default_);
  }
  virtual void TearDown() OVERRIDE {
  }

  bool IsBuiltInDefault(const std::vector<std::string>& apps) {
    if (apps.size() != built_in_default_.size())
      return false;

    for (size_t i = 0; i < built_in_default_.size(); ++i) {
      if (built_in_default_[i] != apps[i])
        return false;
    }

    return true;
  }

  void SetExternalFile(const base::FilePath& path) {
    path_override_.reset(new base::ScopedPathOverride(
        chromeos::FILE_DEFAULT_APP_ORDER, path));
  }

  void CreateExternalOrderFile(const std::string& content) {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath external_file = temp_dir_.path().Append(kTestFile);
    file_util::WriteFile(external_file, content.c_str(), content.size());
    SetExternalFile(external_file);
  }

 private:
  std::vector<std::string> built_in_default_;

  base::ScopedTempDir temp_dir_;
  scoped_ptr<base::ScopedPathOverride> path_override_;

  DISALLOW_COPY_AND_ASSIGN(DefaultAppOrderTest);
};

// Tests that the built-in default order is returned when ExternalLoader is not
// created.
TEST_F(DefaultAppOrderTest, BuiltInDefault) {
  std::vector<std::string> apps;
  default_app_order::Get(&apps);
  EXPECT_TRUE(IsBuiltInDefault(apps));
}

// Tests external order file overrides built-in default.
TEST_F(DefaultAppOrderTest, ExternalOrder) {
  const char kExternalOrder[] = "[\"app1\",\"app2\",\"app3\"]";
  CreateExternalOrderFile(std::string(kExternalOrder));

  scoped_ptr<default_app_order::ExternalLoader> loader(
      new default_app_order::ExternalLoader(false));

  std::vector<std::string> apps;
  default_app_order::Get(&apps);
  EXPECT_EQ(3u, apps.size());
  EXPECT_EQ(std::string("app1"), apps[0]);
  EXPECT_EQ(std::string("app2"), apps[1]);
  EXPECT_EQ(std::string("app3"), apps[2]);
}

// Tests none-existent order file gives built-in default.
TEST_F(DefaultAppOrderTest, NoExternalFile) {
  base::ScopedTempDir scoped_tmp_dir;
  ASSERT_TRUE(scoped_tmp_dir.CreateUniqueTempDir());

  base::FilePath none_existent_file =
      scoped_tmp_dir.path().AppendASCII("none_existent_file");
  ASSERT_FALSE(file_util::PathExists(none_existent_file));
  SetExternalFile(none_existent_file);

  scoped_ptr<default_app_order::ExternalLoader> loader(
      new default_app_order::ExternalLoader(false));

  std::vector<std::string> apps;
  default_app_order::Get(&apps);
  EXPECT_TRUE(IsBuiltInDefault(apps));
}

// Tests bad json file gives built-in default.
TEST_F(DefaultAppOrderTest, BadExternalFile) {
  const char kExternalOrder[] = "This is not a valid json.";
  CreateExternalOrderFile(std::string(kExternalOrder));

  scoped_ptr<default_app_order::ExternalLoader> loader(
      new default_app_order::ExternalLoader(false));

  std::vector<std::string> apps;
  default_app_order::Get(&apps);
  EXPECT_TRUE(IsBuiltInDefault(apps));
}

}  // namespace chromeos
