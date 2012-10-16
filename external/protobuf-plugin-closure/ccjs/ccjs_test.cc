// Copyright (c) 2011 SameGoal LLC.
// All Rights Reserved.
// Author: Andy Hochhaus <ahochhaus@samegoal.com>

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdlib.h>
#include <gtest/gtest.h>

#include <string>

#include "base/init.h"
#include "protobuf/js/test.pb.h"
#include "protobuf/js/package_test.pb.h"

void PopulateMessage(TestAllTypes *message) {
  message->set_optional_int32(101);
  message->set_optional_int64(102);
  message->set_optional_uint32(103);
  message->set_optional_uint64(104);
  message->set_optional_sint32(105);
  message->set_optional_sint64(106);
  message->set_optional_fixed32(107);
  message->set_optional_fixed64(108);
  message->set_optional_sfixed32(109);
  message->set_optional_sfixed64(110);
  message->set_optional_float(111.5);
  message->set_optional_double(112.5);
  message->set_optional_bool(true);
  message->set_optional_string("test");
  message->set_optional_bytes("abcd");

  TestAllTypes_OptionalGroup *group = message->mutable_optionalgroup();
  group->set_a(111);

  TestAllTypes_NestedMessage *nestedMessage =
      message->mutable_optional_nested_message();
  nestedMessage->set_b(112);

  message->set_optional_nested_enum(TestAllTypes_NestedEnum_FOO);

  message->add_repeated_int32(201);
  message->add_repeated_int32(202);

  message->add_repeated_string("foo");
  message->add_repeated_string("bar");
}

void ValidateMessage(const TestAllTypes &message) {
  ASSERT_TRUE(message.has_optional_int32());
  ASSERT_TRUE(message.has_optional_int64());
  ASSERT_TRUE(message.has_optional_uint32());
  ASSERT_TRUE(message.has_optional_uint64());
  ASSERT_TRUE(message.has_optional_sint32());
  ASSERT_TRUE(message.has_optional_sint64());
  ASSERT_TRUE(message.has_optional_fixed32());
  ASSERT_TRUE(message.has_optional_fixed64());
  ASSERT_TRUE(message.has_optional_sfixed32());
  ASSERT_TRUE(message.has_optional_sfixed64());
  ASSERT_TRUE(message.has_optional_float());
  ASSERT_TRUE(message.has_optional_double());
  ASSERT_TRUE(message.has_optional_bool());
  ASSERT_TRUE(message.has_optional_string());
  ASSERT_TRUE(message.has_optional_bytes());
  ASSERT_TRUE(message.has_optionalgroup());
  ASSERT_TRUE(message.has_optional_nested_message());
  ASSERT_TRUE(message.has_optional_nested_enum());

  ASSERT_EQ(2, message.repeated_int32_size());
  ASSERT_EQ(0, message.repeated_int64_size());
  ASSERT_EQ(0, message.repeated_uint32_size());
  ASSERT_EQ(0, message.repeated_uint64_size());
  ASSERT_EQ(0, message.repeated_sint32_size());
  ASSERT_EQ(0, message.repeated_sint64_size());
  ASSERT_EQ(0, message.repeated_fixed32_size());
  ASSERT_EQ(0, message.repeated_fixed64_size());
  ASSERT_EQ(0, message.repeated_sfixed32_size());
  ASSERT_EQ(0, message.repeated_sfixed64_size());
  ASSERT_EQ(0, message.repeated_float_size());
  ASSERT_EQ(0, message.repeated_double_size());
  ASSERT_EQ(0, message.repeated_bool_size());
  ASSERT_EQ(2, message.repeated_string_size());
  ASSERT_EQ(0, message.repeated_bytes_size());
  ASSERT_EQ(0, message.repeatedgroup_size());
  ASSERT_EQ(0, message.repeated_nested_message_size());
  ASSERT_EQ(0, message.repeated_nested_enum_size());

  ASSERT_EQ(101, message.optional_int32());
  ASSERT_EQ(102, message.optional_int64());
  ASSERT_EQ(static_cast<unsigned int>(103), message.optional_uint32());
  ASSERT_EQ(static_cast<unsigned int>(104), message.optional_uint64());
  ASSERT_EQ(105, message.optional_sint32());
  ASSERT_EQ(106, message.optional_sint64());
  ASSERT_EQ(static_cast<unsigned int>(107), message.optional_fixed32());
  ASSERT_EQ(static_cast<unsigned int>(108), message.optional_fixed64());
  ASSERT_EQ(109, message.optional_sfixed32());
  ASSERT_EQ(110, message.optional_sfixed64());
  ASSERT_EQ(111.5, message.optional_float());
  ASSERT_EQ(112.5, message.optional_double());
  ASSERT_EQ(true, message.optional_bool());
  ASSERT_EQ("test", message.optional_string());
  ASSERT_EQ("abcd", message.optional_bytes());
  ASSERT_EQ(111, message.optionalgroup().a());
  ASSERT_EQ(112, message.optional_nested_message().b());
  ASSERT_EQ(TestAllTypes_NestedEnum_FOO, message.optional_nested_enum());
  ASSERT_EQ(201, message.repeated_int32(0));
  ASSERT_EQ(202, message.repeated_int32(1));
}

// golden values were extracted from closure-library unit tests.
const std::string pblite_golden =
    "[null,101,\"102\",103,\"104\",105,\"106\",107,\"108\",109,\"110\","
    "111.5,112.5,1,\"test\",\"abcd\",[null,null,null,null,null,null,null,"
    "null,null,null,null,null,null,null,null,null,null,111],null,[null,112],"
    "null,null,0,null,null,null,null,null,null,null,null,null,[201,202],"
    "[],[],[],[],[],[],[],[],[],[],[],[],[\"foo\",\"bar\"]]";

const std::string pblite_zero_index_golden =
    "[101,\"102\",103,\"104\",105,\"106\",107,\"108\",109,\"110\","
    "111.5,112.5,1,\"test\",\"abcd\",[null,null,null,null,null,null,"
    "null,null,null,null,null,null,null,null,null,null,111],null,[112],"
    "null,null,0,null,null,null,null,null,null,null,null,null,[201,202],"
    "[],[],[],[],[],[],[],[],[],[],[],[],[\"foo\",\"bar\"]]";

const std::string large_int_pblite_golden =
    "[null,null,null,null,\"1000000000000000001\",null,null,null,null,null,"
    "null,null,null,null,null,null,null,null,null,null,null,null,null,null,"
    "null,null,null,null,null,null,null,[],[],[],[],[],[],[],[],[],[],[],[],"
    "[],[],[],[],null,[],[],1000000000000000001,\"1000000000000000001\"]";

const std::string large_int_pblite_zero_index_golden =
    "[null,null,null,\"1000000000000000001\",null,null,null,null,null,"
    "null,null,null,null,null,null,null,null,null,null,null,null,null,null,"
    "null,null,null,null,null,null,null,[],[],[],[],[],[],[],[],[],[],[],[],"
    "[],[],[],[],null,[],[],1000000000000000001,\"1000000000000000001\"]";

const std::string pblite_package_golden = "[null,1," + pblite_golden + "]";

const std::string pblite_package_zero_index_golden =
    "[1," + pblite_zero_index_golden + "]";

const std::string object_key_name_golden =
    "{\"optional_int32\":101,\"optional_int64\":\"102\","
    "\"optional_uint32\":103,\"optional_uint64\":\"104\","
    "\"optional_sint32\":105,\"optional_sint64\":\"106\","
    "\"optional_fixed32\":107,\"optional_fixed64\":\"108\","
    "\"optional_sfixed32\":109,\"optional_sfixed64\":\"110\","
    "\"optional_float\":111.5,\"optional_double\":112.5,"
    "\"optional_bool\":true,\"optional_string\":\"test\","
    "\"optional_bytes\":\"abcd\",\"optionalgroup\":{\"a\":111},"
    "\"optional_nested_message\":{\"b\":112},\"optional_nested_enum\":0,"
    "\"repeated_int32\":[201,202],\"repeated_string\":[\"foo\",\"bar\"]}";

const std::string large_int_object_key_name_golden =
    "{\"optional_uint64\":\"1000000000000000001\","
    "\"optional_int64_number\":1000000000000000001,"
    "\"optional_int64_string\":\"1000000000000000001\"}";

const std::string object_key_name_package_golden =
    "{\"optional_int32\":1,\"other_all\":" + object_key_name_golden + "}";

const std::string object_key_tag_golden =
    "{\"1\":101,\"2\":\"102\",\"3\":103,\"4\":\"104\",\"5\":105,"
    "\"6\":\"106\",\"7\":107,\"8\":\"108\",\"9\":109,\"10\":\"110\","
    "\"11\":111.5,\"12\":112.5,\"13\":true,\"14\":\"test\","
    "\"15\":\"abcd\",\"16\":{\"17\":111},\"18\":{\"1\":112},\"21\":0,"
    "\"31\":[201,202],\"44\":[\"foo\",\"bar\"]}";

const std::string large_int_object_key_tag_golden =
    "{\"4\":\"1000000000000000001\","
    "\"50\":1000000000000000001,"
    "\"51\":\"1000000000000000001\"}";

const std::string object_key_tag_package_golden =
    "{\"1\":1,\"2\":" + object_key_tag_golden + "}";

const std::string special_char_string = "\"\\/\b\f\n\r\tÄúɠ";
const std::string object_key_tag_escapes_golden =
    "{\"14\":\"\\\"\\\\/\\b\\f\\n\\r\\t\\u00c4\\u00fa\\u0260\","
    "\"15\":\"\\\"\\\\/\\b\\f\\n\\r\\t\\u00c4\\u00fa\\u0260\"}";

TEST(PbLite, Serialization) {
  TestAllTypes message;
  PopulateMessage(&message);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToPbLiteString(&serialized));
  ASSERT_EQ(pblite_golden, serialized);
}

TEST(PbLite, LargeIntSerialization) {
  TestAllTypes message;
  message.set_optional_uint64(1000000000000000001);
  message.set_optional_int64_number(1000000000000000001);
  message.set_optional_int64_string(1000000000000000001);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToPbLiteString(&serialized));
  ASSERT_EQ(large_int_pblite_golden, serialized);
}

TEST(PbLite, PackageSerialization) {
  someprotopackage::TestPackageTypes message;
  message.set_optional_int32(1);
  auto test_message = message.mutable_other_all();
  PopulateMessage(test_message);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToPbLiteString(&serialized));
  ASSERT_EQ(pblite_package_golden, serialized);
}

TEST(PbLite, Deserialization) {
  TestAllTypes message;
  ASSERT_TRUE(message.ParsePartialFromPbLiteString(pblite_golden));
  ValidateMessage(message);
}

TEST(PbLite, LargeIntDeserialization) {
  TestAllTypes message;
  ASSERT_TRUE(message.ParsePartialFromPbLiteString(large_int_pblite_golden));
  ASSERT_EQ(1000000000000000001, message.optional_uint64());
  ASSERT_EQ(1000000000000000001, message.optional_int64_number());
  ASSERT_EQ(1000000000000000001, message.optional_int64_string());
}

TEST(PbLite, PackageDeserialization) {
  someprotopackage::TestPackageTypes message;
  ASSERT_TRUE(message.ParsePartialFromPbLiteString(pblite_package_golden));
  ASSERT_EQ(1, message.optional_int32());
  ValidateMessage(message.other_all());
}

TEST(PbLiteZeroIndex, Serialization) {
  TestAllTypes message;
  PopulateMessage(&message);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToPbLiteZeroIndexString(&serialized));
  ASSERT_EQ(pblite_zero_index_golden, serialized);
}

TEST(PbLiteZeroIndex, LargeIntSerialization) {
  TestAllTypes message;
  message.set_optional_uint64(1000000000000000001);
  message.set_optional_int64_number(1000000000000000001);
  message.set_optional_int64_string(1000000000000000001);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToPbLiteZeroIndexString(&serialized));
  ASSERT_EQ(large_int_pblite_zero_index_golden, serialized);
}

TEST(PbLiteZeroIndex, PackageSerialization) {
  someprotopackage::TestPackageTypes message;
  message.set_optional_int32(1);
  auto test_message = message.mutable_other_all();
  PopulateMessage(test_message);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToPbLiteZeroIndexString(&serialized));
  ASSERT_EQ(pblite_package_zero_index_golden, serialized);
}

TEST(PbLiteZeroIndex, Deserialization) {
  TestAllTypes message;
  ASSERT_TRUE(message.ParsePartialFromPbLiteZeroIndexString(
      pblite_zero_index_golden));
  ValidateMessage(message);
}

TEST(PbLiteZeroIndex, LargeIntDeserialization) {
  TestAllTypes message;
  ASSERT_TRUE(message.ParsePartialFromPbLiteZeroIndexString(
      large_int_pblite_zero_index_golden));
  ASSERT_EQ(1000000000000000001, message.optional_uint64());
  ASSERT_EQ(1000000000000000001, message.optional_int64_number());
  ASSERT_EQ(1000000000000000001, message.optional_int64_string());
}

TEST(PbLiteZeroIndex, PackageDeserialization) {
  someprotopackage::TestPackageTypes message;
  ASSERT_TRUE(message.ParsePartialFromPbLiteZeroIndexString(
      pblite_package_zero_index_golden));
  ASSERT_EQ(1, message.optional_int32());
  ValidateMessage(message.other_all());
}

TEST(ObjectKeyName, Serialization) {
  TestAllTypes message;
  PopulateMessage(&message);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToObjectKeyNameString(&serialized));
  ASSERT_EQ(object_key_name_golden, serialized);
}

TEST(ObjectKeyName, LargeIntSerialization) {
  TestAllTypes message;
  message.set_optional_uint64(1000000000000000001);
  message.set_optional_int64_number(1000000000000000001);
  message.set_optional_int64_string(1000000000000000001);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToObjectKeyNameString(&serialized));
  ASSERT_EQ(large_int_object_key_name_golden, serialized);
}

TEST(ObjectKeyName, PackageSerialization) {
  someprotopackage::TestPackageTypes message;
  message.set_optional_int32(1);
  auto test_message = message.mutable_other_all();
  PopulateMessage(test_message);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToObjectKeyNameString(&serialized));
  ASSERT_EQ(object_key_name_package_golden, serialized);
}

TEST(ObjectKeyName, Deserialization) {
  TestAllTypes message;
  ASSERT_TRUE(
      message.ParsePartialFromObjectKeyNameString(object_key_name_golden));
  ValidateMessage(message);
}

TEST(ObjectKeyName, LargeIntDeserialization) {
  TestAllTypes message;
  ASSERT_TRUE(message.ParsePartialFromObjectKeyNameString(
      large_int_object_key_name_golden));
  ASSERT_EQ(1000000000000000001, message.optional_uint64());
  ASSERT_EQ(1000000000000000001, message.optional_int64_number());
  ASSERT_EQ(1000000000000000001, message.optional_int64_string());
}

TEST(ObjectKeyName, PackageDeserialization) {
  someprotopackage::TestPackageTypes message;
  ASSERT_TRUE(message.ParsePartialFromObjectKeyNameString(
      object_key_name_package_golden));
  ASSERT_EQ(1, message.optional_int32());
  ValidateMessage(message.other_all());
}

TEST(ObjectKeyTag, Serialization) {
  TestAllTypes message;
  PopulateMessage(&message);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToObjectKeyTagString(&serialized));
  ASSERT_EQ(object_key_tag_golden, serialized);
}

TEST(ObjectKeyTag, LargeIntSerialization) {
  TestAllTypes message;
  message.set_optional_uint64(1000000000000000001);
  message.set_optional_int64_number(1000000000000000001);
  message.set_optional_int64_string(1000000000000000001);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToObjectKeyTagString(&serialized));
  ASSERT_EQ(large_int_object_key_tag_golden, serialized);
}

TEST(ObjectKeyTag, PackageSerialization) {
  someprotopackage::TestPackageTypes message;
  message.set_optional_int32(1);
  auto test_message = message.mutable_other_all();
  PopulateMessage(test_message);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToObjectKeyTagString(&serialized));
  ASSERT_EQ(object_key_tag_package_golden, serialized);
}

TEST(ObjectKeyTag, Deserialization) {
  TestAllTypes message;
  ASSERT_TRUE(
      message.ParsePartialFromObjectKeyTagString(object_key_tag_golden));
  ValidateMessage(message);
}

TEST(ObjectKeyTag, LargeIntDeserialization) {
  TestAllTypes message;
  ASSERT_TRUE(message.ParsePartialFromObjectKeyTagString(
      large_int_object_key_tag_golden));
  ASSERT_EQ(1000000000000000001, message.optional_uint64());
  ASSERT_EQ(1000000000000000001, message.optional_int64_number());
  ASSERT_EQ(1000000000000000001, message.optional_int64_string());
}

TEST(ObjectKeyTag, PackageDeserialization) {
  someprotopackage::TestPackageTypes message;
  ASSERT_TRUE(message.ParsePartialFromObjectKeyTagString(
      object_key_tag_package_golden));
  ASSERT_EQ(1, message.optional_int32());
  ValidateMessage(message.other_all());
}

TEST(ObjectKeyTag, EscapeSerialization) {
  TestAllTypes message;
  message.set_optional_string(special_char_string);
  message.set_optional_bytes(special_char_string);

  std::string serialized;
  ASSERT_TRUE(message.SerializePartialToObjectKeyTagString(&serialized));
  ASSERT_EQ(object_key_tag_escapes_golden, serialized);
}

TEST(ObjectKeyTag, EscapeDeserialization) {
  TestAllTypes message;
  ASSERT_TRUE(
      message.ParsePartialFromObjectKeyTagString(
          object_key_tag_escapes_golden));

  ASSERT_TRUE(message.has_optional_string());
  ASSERT_TRUE(message.has_optional_bytes());
  ASSERT_EQ(special_char_string, message.optional_string());
  ASSERT_EQ(special_char_string, message.optional_bytes());
}

const char *usage = "ccjs_test\n";

int main(int argc, char **argv) {
  ::sg::Init(usage, &argc, &argv, true);
  return RUN_ALL_TESTS();
}
