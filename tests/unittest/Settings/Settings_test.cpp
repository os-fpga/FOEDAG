/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Main/Settings.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <fstream>

#include "Main/Tasks.h"
#include "Main/WidgetFactory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;

namespace FOEDAG {
namespace {

TEST(Settings, LoadSettingsMergesJson) {
  Settings test;

  // The goal of this test is to load two json files w/ different child nodes
  // and ensure that the two files are combined

  // This implicitly tests Settings::loadJsonFile() which is used by
  // loadSettings() to load json files

  QStringList files{":/Settings/settings_loadJson_1.json",
                    ":/Settings/settings_loadJson_2.json"};
  test.loadSettings(files);
  json gold = R"(
    {
        "Nested1": {
            "Nested2": {
                "TestCategory": {
                    "_META_": {
                        "metaValue1": 1,
                        "metaValue2": 2
                    }
                }
            }
        }
    }
    )"_json;

  EXPECT_EQ(gold, test.getJson()) << "Ensure that Settings::loadSettings() "
                                     "combines similarly nested json objects";
}

TEST(Settings, JsonIsEditable) {
  Settings test;
  json empty;

  // The goal of this test is to ensure that the json stored in Settings is
  // modifiable

  EXPECT_EQ(empty, test.getJson())
      << "Ensure that the json returned by a new Settings object is empty #1";
  EXPECT_EQ(test.getJson().empty(), true)
      << "Ensure that the json returned by a new Settings object is empty #2";

  // take a copy and ref of the current json which is empty if the above tests
  // passed
  json copy = test.getJson();
  json& ref = test.getJson();

  json editGold = R"( {"SomeNewValue" : 123} )"_json;
  test.getJson()["SomeNewValue"] = 123;

  EXPECT_EQ(test.getJson(), editGold)
      << "Ensure that directly editing the return value of getJson() updates "
         "the json stored in Settings";

  EXPECT_EQ(copy, empty)
      << "Ensure that a copy of the json does NOT see "
         "updates to the settings json after a change is made";

  EXPECT_EQ(test.getJson(), ref)
      << "Ensure that a ref of the json DOES see updates to the settings json "
         "after a change is made";

  EXPECT_EQ(test.getJson().empty(), false)
      << "Ensure that the json can be cleared 1/2";
  test.clear();
  EXPECT_EQ(test.getJson().empty(), true)
      << "Ensure that the json can be cleared 2/2";
}

TEST(Settings, TraverseJsonWorksForObjectsAndArrays) {
  // The goal of this test is to ensure that the traverseJson functionality can
  // traverse nested objects and arrays

  // This test will search the json for a "_META_" tag and read it's "metaVal"
  // to demonstrate the traversal

  json testJson = R"(
    {
        "l1": {
            "_META_": { "metaVal" : 1 },
            "array" : [
                { "l1.1" : {"_META_": { "metaVal" : 1.1 }}},
                { "l1.2" : {"_META_": { "metaVal" : 1.2 }}, "l1.2.1" : {"_META_":{"metaVal" : 1.21}}}
            ],
            "object": {
                "l3": {"_META_": { "metaVal": 3 }}
            }
        },
        "_META_": { "metaVal" : 0 }
    }
    )"_json;

  QStringList gold{": 0",
                   "/l1: 1",
                   "/l1/array/0/l1.1: 1.1",
                   "/l1/array/1/l1.2: 1.2",
                   "/l1/array/1/l1.2.1: 1.21",
                   "/l1/object/l3: 3"};

  QStringList results;
  auto findCb = [&results](json& obj, const QString& path) {
    // If the object contains a _META_ tag, lookup "metaVal" and store it along
    // with the json ptr path
    if (obj.contains("_META_")) {
      QString result =
          path + ": " + QString::number(obj["_META_"].value("metaVal", -1.0));
      results << result;
    }
  };
  Settings::traverseJson(testJson, findCb);

  EXPECT_EQ(gold, results)
      << "Ensure that traverseJson found the _META_ tags and values";
}

TEST(Settings, GetLookupValueWorks) {
  // The goal of this test is to ensure that getLookupValue can lookup two
  // string lists and perform a lookup between the two
  json testJson = R"(
              {
                "options": [
                    "B1",
                    "B2",
                    "B3"
                ],
                "optionsLookup": [
                    "b1",
                    "b2",
                    "b3"
                ],
                "displayNames": [
                    "Title 1",
                    "Title 2",
                    "Title 3"
                ],
                "lookupVals": [
                    "_back_end_val_1_",
                    "_back_end_val_2_",
                    "_back_end_val_3_"
                ]
            })"_json;

  EXPECT_EQ("", Settings::getLookupValue(testJson, "B4"))
      << "Ensure an unfound option returns \"\"";
  EXPECT_EQ("", Settings::getLookupValue(testJson, "B2", "options", "BadValue"))
      << "Ensure a bad lookupArrayKey returns \"\"";
  ;
  EXPECT_EQ(
      "", Settings::getLookupValue(testJson, "B2", "BadValue", "optionsLookup"))
      << "Ensure a bad optionsArrayKey returns \"\"";
  ;

  EXPECT_EQ("b3", Settings::getLookupValue(testJson, "B3"))
      << "Ensure lookup defaults to options/optionsLookup pair if no keys are "
         "provided";
  EXPECT_EQ("_back_end_val_2_",
            Settings::getLookupValue(testJson, "Title 2", "displayNames",
                                     "lookupVals"));
}

TEST(Settings, GetTclArgString) {
  Settings test;
  // The goal of this test is to ensure that getTclArgString properly parses a
  // tcl arg list from a given a json object containing widget factory fields
  // In particular, this test helps ensure that number fields like Spinbox
  // and DoubleSpinbox properly can read number values as well as only provide
  // an arg tag w/ no value when a checkbox is checked.

  QString jsonStr{};
  QFile file(":/Settings/settings_tclExample_defaults.json");
  if (file.open(QFile::ReadOnly | QFile::Text)) {
    QTextStream in(&file);
    jsonStr = in.readAll();
  }

  json data = json::parse(jsonStr.toStdString());
  QString tclArgs = Settings::getTclArgString(data["Tasks"]["TclExample"]);
  QString expectedVals =
      " -double_spin_ex 2.5 -int_spin_ex1 2 -int_spin_ex2 3 "
      "-radio_ex b2 -check_ex1 true -check_ex2 false -dropdown_ex "
      "option2 -input_ex some_text";
  EXPECT_EQ(tclArgs.toStdString(), expectedVals.toStdString())
      << "Ensure tclArgsString properly parses widget factory json fields";
}

TEST(Settings, TclValsInit) {
  Settings test;
  // The goal of this test is to ensure that settings json that contains a
  // tclArgKey in its _META_ tag and supplies an "arg" value for a field will
  // have its tcl values available for reading after load (Originally the
  // values wouldn't be available until the settings dlg was opened and saved)

  // Grab the getter we will use to verify the tcl values are being set
  auto [setter, getter] = FOEDAG::getTclArgFns("TclExample");

  // Load some default values w/ no userValues specified
  test.loadSettings(
      QStringList{":/Settings/settings_tclExample_defaults.json"});

  // These pairs should match the "arg" and "default" fields of
  // settings_tclExample_defaults.json
  // Note that some fields do some automatic formatting like an int field
  // truncating a double to an int or a checkbox not reporting a value and
  // instead using the presence or absence of the tag to represent if it's
  // toggled on or off.
  QString expectedVals =
      "-double_spin_ex 2.5 -int_spin_ex1 2 -int_spin_ex2 3 "
      "-radio_ex b2 -check_ex1 true -check_ex2 false -dropdown_ex "
      "option2 -input_ex some_text";

  EXPECT_EQ(expectedVals.toStdString(), getter().toString())
      << "Ensure the default TclExample values are reported";

  // Load json w/ "userValues" set to simulate saved user settings
  test.loadSettings(
      QStringList{":/Settings/settings_tclExample_userVals.json"});

  // Now we expect the userValue's to be reported instead of default
  expectedVals =
      "-double_spin_ex 4.5 -int_spin_ex1 4 -int_spin_ex2 4 "
      "-radio_ex b1 -check_ex1 false -check_ex2 true -dropdown_ex "
      "option1 -input_ex new_text";

  EXPECT_EQ(expectedVals.toStdString(), getter().toString())
      << "Ensure the userValue TclExample values are reported";
}

}  // namespace
}  // namespace FOEDAG
