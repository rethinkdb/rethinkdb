// Copyright (c) 2010-2011 SameGoal LLC.
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

// Design Considerations
//
// Protocol Buffers can be extended to support new (de)serialization formats in
// at least the following ways:
//
// 1) Reflection based library (eg: TextFormat).
//    Use protobuf reflection in an external library (not generated code) to
//    (de)serialize messages.
//    Pro: no code generation plugins are required.
//    Con: reflection requires "full" (not lite) protobuf support.
//    Con: code speed is slower.
//    Con: TextFormat is not thread safe and therefore may not be an ideal base
//         class for "text based" (eg: json, xml) (de)serializations. This
//         limitation could be worked around by not sub-classing TextFormat.
//
// 2) Subclass Coded{Input,Output}Stream to provide alternate implementations
//    of the {Read,Write}* methods which could be passed in to the upstream
//    {Parse,Serialize} routines at the ParseFromCodedStream level.
//    Pro: no generated code plugins are required.
//    Pro: no reflection is required so protobuf lite support is sufficient.
//    Pro: Parser/Serializer framework from protobuf is reused.
//    Pro: good code speed.
//    Con: the protobuf parser is already written and could be somewhat
//         limiting of the types of serialization formats supported.
//         Additionally, implementing the {Read,Write}* methods to match the
//         set of assumptions in the protobuf library could be tricky.
//
// 3) Custom (de)serialization plugin
//    Pro: no reflection is required so protobuf lite support is sufficient.
//    Pro: good code speed.
//    Pro: writing a custom parsing framework is very flexible and allows for
//         any input language/syntax to be supported.
//    Con: ZeroCopy{Input,Output}Stream is low level and tricky to work with.
//    Con: code generation plugin required.
//
// If you have other suggestions for how to add new (de)serialization formats
// to protobuf please email me Andy Hochhaus <ahochhaus@samegoal.com>.
//
// This plugin implements option number 3.

#include "ccjs/code_generator.h"

#include <stdio.h>
#include <string>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

#include "js/int64_encoding.pb.h"

namespace sg {
namespace protobuf {
namespace ccjs {

namespace internal {

std::string ReplaceAll(
    const std::string &from, const std::string &to, const std::string &value) {
  size_t loc;
  std::string rtn = value;
  while ((loc = rtn.find(from)) != std::string::npos) {
    rtn = rtn.replace(loc, from.length(), to);
  }
  return rtn;
}

const std::string cc_header_boilerplate =
    // "#include <iostream>\n"
    "#include <set>\n"
    "#include <stdio.h>\n"
    "\n"
    "#include <google/protobuf/io/zero_copy_stream.h>\n"
    "#include <google/protobuf/io/zero_copy_stream_impl_lite.h>\n"
    "#include <google/protobuf/stubs/common.h>\n"
    "\n"
    "namespace {\n"
    "\n"
    "#define PB_LITE 1\n"
    "#define OBJECT_KEY_NAME 2\n"
    "#define OBJECT_KEY_TAG 3\n"
    "\n"
    // "#define RTN_FALSE std::cout << __FILE__ << \":\"\\\n"
    // "                            << __LINE__ << std::endl; exit(1)\n"
    "#define RTN_FALSE return false\n"
    "\n"
    "bool WriteRaw(const std::string &value,\n"
    "              google::protobuf::io::ZeroCopyOutputStream *output) {\n"
    "  int bytes_remaining = value.length();\n"
    "  while (bytes_remaining) {\n"
    "    void *buffer;\n"
    "    int size;\n"
    "    if (!output->Next(&buffer, &size)) {\n"
    "      RTN_FALSE;\n"
    "    }\n"
    "    const char *value_ptr = value.data() + (\n"
    "        value.length() - bytes_remaining);\n"
    "    if (size >= bytes_remaining) {\n"
    "      memcpy(buffer, value_ptr, bytes_remaining);\n"
    "      int bytes_to_return = size - bytes_remaining;\n"
    "      if (bytes_to_return) {\n"
    "        output->BackUp(bytes_to_return);\n"
    "      }\n"
    "      bytes_remaining = 0;\n"
    "    } else if (size > 0) {\n"
    "      memcpy(buffer, value_ptr, size);\n"
    "      bytes_remaining -= size;\n"
    "    }\n"
    "  }\n"
    "  return true;\n"
    "}\n"
    "\n"
    "bool NextCppCharToJsonEscapedBuffer(\n"
    "    char **src_ptr,\n"
    "    const char *src_end_ptr,\n"
    "    char *json_escaped_buf,\n"
    "    google::protobuf::uint64 *json_escaped_size) {\n"
    "  google::protobuf::uint64 src_len = src_end_ptr - *src_ptr;\n"
    "\n"
    "  // TODO(ahochhaus): json.org escapes '/', but goog.json does not.\n"
    "\n"
    "  // http://en.wikipedia.org/wiki/UTF-8\n"
    "  google::protobuf::uint64 src_bytes = 0;\n"
    "  char init_mask = 0;\n"
    "  if ((**src_ptr & 0b10000000) == 0b00000000 && src_len >= 1) {\n"
    "    // one byte sequence\n"
    "    // 0xxxxxxx -> 0xxxxxxx\n"
    "    const char c = *(*src_ptr)++ & 0b01111111;\n"
    "    switch (c) {\n"
    "      case '\"':\n"
    "      case '\\\\':\n"
    "        json_escaped_buf[0] = '\\\\';\n"
    "        json_escaped_buf[1] = c;\n"
    "        *json_escaped_size = 2;\n"
    "        break;\n"
    "      case '\\b':\n"
    "        json_escaped_buf[0] = '\\\\';\n"
    "        json_escaped_buf[1] = 'b';\n"
    "        *json_escaped_size = 2;\n"
    "        break;\n"
    "      case '\\f':\n"
    "        json_escaped_buf[0] = '\\\\';\n"
    "        json_escaped_buf[1] = 'f';\n"
    "        *json_escaped_size = 2;\n"
    "        break;\n"
    "      case '\\n':\n"
    "        json_escaped_buf[0] = '\\\\';\n"
    "        json_escaped_buf[1] = 'n';\n"
    "        *json_escaped_size = 2;\n"
    "        break;\n"
    "      case '\\r':\n"
    "        json_escaped_buf[0] = '\\\\';\n"
    "        json_escaped_buf[1] = 'r';\n"
    "        *json_escaped_size = 2;\n"
    "        break;\n"
    "      case '\\t':\n"
    "        json_escaped_buf[0] = '\\\\';\n"
    "        json_escaped_buf[1] = 't';\n"
    "        *json_escaped_size = 2;\n"
    "        break;\n"
    "      default:\n"
    "        json_escaped_buf[0] = c;\n"
    "        *json_escaped_size = 1;\n"
    "        break;\n"
    "    }\n"
    "    return true;\n"
    "  } else if ((**src_ptr & 0b11100000) == 0b11000000 &&\n"
    "             src_len >= 2) {\n"
    "    // two byte sequence\n"
    "    // 110yyyyy 10xxxxxx -> 00000yyy yyxxxxxx\n"
    "    src_bytes = 2;\n"
    "    init_mask = 0b00011111;\n"
    "  } else if ((**src_ptr & 0b11110000) == 0b11100000 &&\n"
    "             src_len >= 3) {\n"
    "    // three byte sequence\n"
    "    // 1110zzzz 10yyyyyy 10xxxxxx -> zzzzyyyy yyxxxxxx\n"
    "    src_bytes = 3;\n"
    "    init_mask = 0b00001111;\n"
    "  } else if ((**src_ptr & 0b11111000) == 0b11110000 &&\n"
    "             src_len >= 4) {\n"
    "    // four byte sequence\n"
    "    // 11110www 10zzzzzz 10yyyyyy 10xxxxxx -> "  // no newline
    "000wwwzz zzzzyyyy yyxxxxxx\n"
    "    src_bytes = 4;\n"
    "    init_mask = 0b00000111;\n"
    "  } else {\n"
    "    // illegal encoding\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "\n"
    "  google::protobuf::uint64 val = *(*src_ptr)++ & init_mask;\n"
    "  for (google::protobuf::uint64 i = 1; i < src_bytes; ++i) {\n"
    "    val = val << 6;\n"
    "    val |= *(*src_ptr)++ & 0b00111111;\n"
    "  }\n"
    "\n"
    "  if (snprintf(json_escaped_buf,\n"
    "               sizeof(json_escaped_buf),\n"
    "               \"\\\\u%04lx\",\n"
    "               val) != 6) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  *json_escaped_size = 6;\n"
    "\n"
    "  return true;\n"
    "}\n"
    "bool WriteEscaped(\n"
    "    const std::string &value,\n"
    "    google::protobuf::io::ZeroCopyOutputStream *output) {\n"
    "  char *src_ptr = const_cast<char *> (value.data());\n"
    "  const char *src_end_ptr = src_ptr + value.length();\n"
    "  std::string json_escaped_str;\n"
    "  while (src_ptr < src_end_ptr) {\n"
    "    char json_escaped_buf[7];\n"
    "    google::protobuf::uint64 json_escaped_size;\n"
    "    if (!NextCppCharToJsonEscapedBuffer(\n"
    "      &src_ptr, src_end_ptr, json_escaped_buf, &json_escaped_size)) {\n"
    "      RTN_FALSE;\n"
    "    }\n"
    "    json_escaped_str += std::string(json_escaped_buf,\n"
    "                                    json_escaped_size);\n"
    "  }\n"
    "  if (!WriteRaw(json_escaped_str, output)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  return true;\n"
    "}\n"
    "\n"
    "bool WriteString(\n"
    "    const std::string &value,\n"
    "    google::protobuf::io::ZeroCopyOutputStream *output) {\n"
    "  if (!WriteRaw(\"\\\"\", output)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!WriteEscaped(value, output)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!WriteRaw(\"\\\"\", output)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  return true;\n"
    "}\n"
    "\n"
    "bool WritePbLiteNullEntries(\n"
    "    const google::protobuf::uint32 field_num,\n"
    "    const std::set<google::protobuf::uint32> repeated_field_set,\n"
    "    const bool start_index_one,\n"
    "    google::protobuf::uint32 *cur_field_num,\n"
    "    google::protobuf::io::ZeroCopyOutputStream *output) {\n"
    "  if (*cur_field_num > field_num) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "\n"
    "  while (*cur_field_num < field_num) {\n"
    "    std::string write_str;\n"
    "    if (!start_index_one) {\n"
    "      write_str = (*cur_field_num != 0) ? \",\" : \"\";\n"
    "    } else {\n"
    "      write_str = (*cur_field_num != 1) ? \",\" : \"\";\n"
    "    }\n"
    "    if (repeated_field_set.find(*cur_field_num) ==\n"
    "        repeated_field_set.end()) {\n"
    "      write_str += \"null\";\n"
    "    } else {\n"
    "      write_str += \"[]\";\n"
    "    }\n"
    "    if (!WriteRaw(write_str, output)) {\n"
    "      RTN_FALSE;\n"
    "    }\n"
    "    ++(*cur_field_num);\n"
    "  }\n"
    "  if (!start_index_one || *cur_field_num != 1) {\n"
    "    if (!WriteRaw(\",\", output)) {\n"
    "      RTN_FALSE;\n"
    "    }\n"
    "  }\n"
    "  ++(*cur_field_num);\n"
    "  return true;\n"
    "}\n"
    "\n"
    "bool WriteObjectKey(\n"
    "    const std::string &key,\n"
    "    const bool prev_fields,\n"
    "    google::protobuf::io::ZeroCopyOutputStream *output) {\n"
    "  if (prev_fields) {\n"
    "    if (!WriteRaw(\",\", output)) {\n"
    "      RTN_FALSE;\n"
    "    }\n"
    "  }\n"
    "  if (!WriteRaw(\"\\\"\", output)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!WriteRaw(key, output)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!WriteRaw(\"\\\":\", output)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  return true;\n"
    "}\n"
    "\n"
    "enum Token {\n"
    "  TOKEN_NONE,\n"
    "  TOKEN_CURLY_OPEN,\n"
    "  TOKEN_CURLY_CLOSE,\n"
    "  TOKEN_SQUARE_OPEN,\n"
    "  TOKEN_SQUARE_CLOSE,\n"
    "  TOKEN_COLON,\n"
    "  TOKEN_COMMA,\n"
    "  TOKEN_STRING,\n"
    "  TOKEN_NUMBER,\n"
    "  TOKEN_NULL,\n"
    "  TOKEN_TRUE,\n"
    "  TOKEN_FALSE\n"
    "};\n"
    "\n"
    "bool ReadToken(const bool eat_single_char_token,\n"
    "               Token *token,\n"
    "               google::protobuf::io::ZeroCopyInputStream *input) {\n"
    "  char token_buffer[5];\n"
    "  int token_buffer_chars = 0;\n"
    "  *token = TOKEN_NONE;\n"
    "\n"
    "  const void *read_buffer;\n"
    "  int read_size;\n"
    "  while (input->Next(&read_buffer, &read_size)) {\n"
    "    if (read_size > 0) {\n"
    "      int extra_chars_read = read_size - (5 - token_buffer_chars);\n"
    "      if (extra_chars_read < 0) {\n"
    "        extra_chars_read = 0;\n"
    "      }\n"
    "      int token_size = 0;\n"
    "      for (int i = 0; i < read_size && token_buffer_chars < 5; ++i) {\n"
    "        token_buffer[token_buffer_chars++] = "  // no newline
    "static_cast<const char *> (\n"
    "            read_buffer)[i];\n"
    "      }\n"
    "      switch (token_buffer[0]) {\n"
    "        case '{':\n"
    "          *token = TOKEN_CURLY_OPEN;\n"
    "          token_size = eat_single_char_token ? 1 : 0;\n"
    "          break;\n"
    "        case '}':\n"
    "          *token = TOKEN_CURLY_CLOSE;\n"
    "          token_size = eat_single_char_token ? 1 : 0;\n"
    "          break;\n"
    "        case '[':\n"
    "          *token = TOKEN_SQUARE_OPEN;\n"
    "          token_size = eat_single_char_token ? 1 : 0;\n"
    "          break;\n"
    "        case ']':\n"
    "          *token = TOKEN_SQUARE_CLOSE;\n"
    "          token_size = eat_single_char_token ? 1 : 0;\n"
    "          break;\n"
    "        case ':':\n"
    "          *token = TOKEN_COLON;\n"
    "          token_size = eat_single_char_token ? 1 : 0;\n"
    "          break;\n"
    "        case ',':\n"
    "          *token = TOKEN_COMMA;\n"
    "          token_size = eat_single_char_token ? 1 : 0;\n"
    "          break;\n"
    "        case '\"':\n"
    "          *token = TOKEN_STRING;\n"
    "          token_size = eat_single_char_token ? 1 : 0;\n"
    "          break;\n"
    "        case '-':\n"
    "        case '0':\n"
    "        case '1':\n"
    "        case '2':\n"
    "        case '3':\n"
    "        case '4':\n"
    "        case '5':\n"
    "        case '6':\n"
    "        case '7':\n"
    "        case '8':\n"
    "        case '9':\n"
    "          *token = TOKEN_NUMBER;\n"
    "          token_size = 0;\n"
    "          break;\n"
    "        case 'n':\n"
    "          if (token_buffer_chars >= 4 &&\n"
    "              token_buffer[1] == 'u' &&\n"
    "              token_buffer[2] == 'l' &&\n"
    "              token_buffer[3] == 'l') {\n"
    "            *token = TOKEN_NULL;\n"
    "            token_size = 4;\n"
    "          }\n"
    "          break;\n"
    "        case 't':\n"
    "          if (token_buffer_chars >= 4 &&\n"
    "              token_buffer[1] == 'r' &&\n"
    "              token_buffer[2] == 'u' &&\n"
    "              token_buffer[3] == 'e') {\n"
    "            *token = TOKEN_TRUE;\n"
    "            token_size = 4;\n"
    "          }\n"
    "          break;\n"
    "        case 'f':\n"
    "          if (token_buffer_chars >= 5 &&\n"
    "              token_buffer[1] == 'a' &&\n"
    "              token_buffer[2] == 'l' &&\n"
    "              token_buffer[3] == 's' &&\n"
    "              token_buffer[4] == 'e') {\n"
    "            *token = TOKEN_FALSE;\n"
    "            token_size = 5;\n"
    "          }\n"
    "          break;\n"
    "        default:\n"
    "          RTN_FALSE;\n"
    "          break;\n"
    "      }\n"
    "      if (*token != TOKEN_NONE) {\n"
    "        input->BackUp("  // no newline
    "extra_chars_read + (token_buffer_chars - token_size));\n"
    "        return true;\n"
    "      } else if (token_buffer_chars == 5) {\n"
    "        RTN_FALSE;\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "  if (token_buffer_chars > 0) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  return true;\n"
    "}\n"
    "\n"
    "bool ReadString(std::string *value,\n"
    "                google::protobuf::io::ZeroCopyInputStream *input) {\n"
    "  const void *read_buffer;\n"
    "  char utf8_buf[5];\n"
    "  enum ParseState {\n"
    "    NORMAL,\n"
    "    ESCAPE,\n"
    "    UTF0,\n"
    "    UTF1,\n"
    "    UTF2,\n"
    "    UTF3,\n"
    "  };\n"
    "  ParseState state = NORMAL;\n"
    "  int read_size;\n"
    "  while (input->Next(&read_buffer, &read_size)) {\n"
    "    if (read_size > 0) {\n"
    "      const char *read_buf = static_cast<const char *> (read_buffer);\n"
    "      for (int i = 0; i < read_size; ++i) {\n"
    "        const char cur_char = read_buf[i];\n"
    "        if (state == NORMAL) {\n"
    "          switch (cur_char) {\n"
    "            case '\\\\':\n"
    "              state = ESCAPE;\n"
    "              break;\n"
    "            case '\"':\n"
    "              input->BackUp(read_size - i - 1);\n"
    "              return true;\n"
    "              break;\n"
    "            default:\n"
    "              value->append(1, cur_char);\n"
    "              break;\n"
    "          }\n"
    "        } else if (state == ESCAPE) {\n"
    "          switch (cur_char) {\n"
    "            case '\"':\n"
    "            case '\\\\':\n"
    "              value->append(1, cur_char);\n"
    "              state = NORMAL;\n"
    "              break;\n"
    "            case 'b':\n"
    "              value->append(1, '\\b');\n"
    "              state = NORMAL;\n"
    "              break;\n"
    "            case 'f':\n"
    "              value->append(1, '\\f');\n"
    "              state = NORMAL;\n"
    "              break;\n"
    "            case 'n':\n"
    "              value->append(1, '\\n');\n"
    "              state = NORMAL;\n"
    "              break;\n"
    "            case 'r':\n"
    "              value->append(1, '\\r');\n"
    "              state = NORMAL;\n"
    "              break;\n"
    "            case 't':\n"
    "              value->append(1, '\\t');\n"
    "              state = NORMAL;\n"
    "              break;\n"
    "            case 'u':\n"
    "              state = UTF0;\n"
    "              break;\n"
    "            default:\n"
    "              RTN_FALSE;\n"
    "              break;\n"
    "          }\n"
    "        } else if (state == UTF0) {\n"
    "          utf8_buf[0] = cur_char;\n"
    "          state = UTF1;\n"
    "        } else if (state == UTF1) {\n"
    "          utf8_buf[1] = cur_char;\n"
    "          state = UTF2;\n"
    "        } else if (state == UTF2) {\n"
    "          utf8_buf[2] = cur_char;\n"
    "          state = UTF3;\n"
    "        } else if (state == UTF3) {\n"
    "          utf8_buf[3] = cur_char;\n"
    "          utf8_buf[4] = '\\0';\n"
    "          state = NORMAL;\n"
    "\n"
    "          google::protobuf::uint64 val;\n"
    "          if (sscanf(utf8_buf, \"%04lx\", &val) != 1) {\n"
    "            RTN_FALSE;\n"
    "          }\n"
    "          if (val < 0x00080) {\n"
    "            // one byte sequence\n"
    "            // 0xxxxxxx -> 0xxxxxxx\n"
    "            char output[1];\n"
    "            output[0] = 0b00000000 | (val & 0b01111111);\n"
    "            value->append(output, sizeof(output));\n"
    "          } else if (val < 0x00800) {\n"
    "            // two byte sequence\n"
    "            // 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx\n"
    "            char output[2];\n"
    "            output[0] = 0b11000000 | (val >> 6 & 0b00011111);\n"
    "            output[1] = 0b10000000 | (val & 0b00111111);\n"
    "            value->append(output, sizeof(output));\n"
    "          } else if (val < 0x10000) {\n"
    "            // three byte sequence\n"
    "            // zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx\n"
    "            char output[3];\n"
    "            output[0] = 0b11100000 | (val >> 12 & 0b00001111);\n"
    "            output[1] = 0b10000000 | (val >> 6 & 0b00111111);\n"
    "            output[2] = 0b10000000 | (val & 0b00111111);\n"
    "            value->append(output, sizeof(output));\n"
    "          } else if (val < 0x11000) {\n"
    "            // four byte sequence\n"
    "            // 000wwwzz zzzzyyyy yyxxxxxx -> " // no newline
    "11110www 10zzzzzz 10yyyyyy 10xxxxxx\n"
    "            char output[4];\n"
    "            output[0] = 0b11110000 | (val >> 18 & 0b00000111);\n"
    "            output[1] = 0b10000000 | (val >> 12 & 0b00111111);\n"
    "            output[2] = 0b10000000 | (val >> 6 & 0b00111111);\n"
    "            output[3] = 0b10000000 | (val & 0b00111111);\n"
    "            value->append(output, sizeof(output));\n"
    "          } else {\n"
    "            RTN_FALSE;\n"
    "          }\n"
    "        } else {\n"
    "          RTN_FALSE;\n"
    "        }\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "  RTN_FALSE;\n"
    "}\n"
    "\n"
    "bool ReadNumber(std::string *value,\n"
    "                google::protobuf::io::ZeroCopyInputStream *input) {\n"
    "  const void *read_buffer;\n"
    "  int read_size;\n"
    "  enum State {\n"
    "    PRE_SIGN,\n"
    "    PRE_WHOLE,\n"
    "    WHOLE,\n"
    "    PRE_FRACTION,\n"
    "    FRACTION,\n"
    "    PRE_EXP,\n"
    "    EXP_SIGN,\n"
    "    PRE_EXP_DIGIT,\n"
    "    EXP_DIGIT\n"
    "  };\n"
    "  State state = PRE_SIGN;\n"
    "  while (input->Next(&read_buffer, &read_size)) {\n"
    "    if (read_size > 0) {\n"
    "      const char *read_buf = static_cast<const char *> (read_buffer);\n"
    "      for (int i = 0; i < read_size; ++i) {\n"
    "        char read_char = read_buf[i];\n"
    "        switch (state) {\n"
    "          case PRE_SIGN:\n"
    "            if (read_char == '-') {\n"
    "              state = PRE_WHOLE;\n"
    "            } else if (read_char == '0') {\n"
    "              state = PRE_FRACTION;\n"
    "            } else if (read_char >= '1' && read_char <= '9') {\n"
    "              state = WHOLE;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          case PRE_WHOLE:\n"
    "            if (read_char == '0') {\n"
    "              state = PRE_FRACTION;\n"
    "            } else if (read_char >= '1' && read_char <= '9') {\n"
    "              state = WHOLE;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          case WHOLE:\n"
    "            if (read_char >= '0' && read_char <= '9') {\n"
    "              // state = WHOLE;\n"
    "            } else if (read_char == '.') {\n"
    "              state = FRACTION;\n"
    "            } else if (read_char == 'e' || read_char == 'E') {\n"
    "              state = EXP_SIGN;\n"
    "            } else if (read_char == '\"' ||\n"
    "                       read_char == ',' ||\n"
    "                       read_char == '}' ||\n"
    "                       read_char == ']') {\n"
    "              input->BackUp(read_size - i);\n"
    "              return true;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          case PRE_FRACTION:\n"
    "            if (read_char == '.') {\n"
    "              state = FRACTION;\n"
    "            } else if (read_char == 'e' || read_char == 'E') {\n"
    "              state = EXP_SIGN;\n"
    "            } else if (read_char == '\"' ||\n"
    "                       read_char == ',' ||\n"
    "                       read_char == '}' ||\n"
    "                       read_char == ']') {\n"
    "              input->BackUp(read_size - i);\n"
    "              return true;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          case FRACTION:\n"
    "            if (read_char >= '0' && read_char <= '9') {\n"
    "              // state = FRACTION;\n"
    "            } else if (read_char == 'e' || read_char == 'E') {\n"
    "              state = EXP_SIGN;\n"
    "            } else if (read_char == '\"' ||\n"
    "                       read_char == ',' ||\n"
    "                       read_char == '}' ||\n"
    "                       read_char == ']') {\n"
    "              input->BackUp(read_size - i);\n"
    "              return true;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          case PRE_EXP:\n"
    "            if (read_char == 'e' || read_char == 'E') {\n"
    "              state = EXP_SIGN;\n"
    "            } else if (read_char == '\"' ||\n"
    "                       read_char == ',' ||\n"
    "                       read_char == '}' ||\n"
    "                       read_char == ']') {\n"
    "              input->BackUp(read_size - i);\n"
    "              return true;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          case EXP_SIGN:\n"
    "            if (read_char == '+' || read_char == '-') {\n"
    "              state = PRE_EXP_DIGIT;\n"
    "            } else if (read_char >= '0' && read_char <= '9') {\n"
    "              state = EXP_DIGIT;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          case PRE_EXP_DIGIT:\n"
    "            if (read_char >= '0' && read_char <= '9') {\n"
    "              state = EXP_DIGIT;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          case EXP_DIGIT:\n"
    "            if (read_char >= '0' && read_char <= '9') {\n"
    "              // state = EXP_DIGIT;\n"
    "            } else if (read_char == '\"' ||\n"
    "                       read_char == ',' ||\n"
    "                       read_char == '}' ||\n"
    "                       read_char == ']') {\n"
    "              input->BackUp(read_size - i);\n"
    "              return true;\n"
    "            } else {\n"
    "              RTN_FALSE;\n"
    "            }\n"
    "            break;\n"
    "          default:\n"
    "            RTN_FALSE;\n"
    "            break;\n"
    "        }\n"
    "        value->append(1, read_char);\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "  RTN_FALSE;\n"
    "}\n"
    "\n"
    "bool ReadNumberFromString(\n"
    "    std::string *value,\n"
    "    google::protobuf::io::ZeroCopyInputStream *input) {\n"
    "  Token token;\n"
    "  if (!ReadToken(true, &token, input) || token != TOKEN_STRING) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!ReadNumber(value, input)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!ReadToken(true, &token, input) || token != TOKEN_STRING) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  return true;\n"
    "}\n"
    "\n"
    "bool ReadPbLiteNextTag(\n"
    "    google::protobuf::int32 *cur_field_num,\n"
    "    Token *token,\n"
    "    google::protobuf::io::ZeroCopyInputStream *input) {\n"
    "  while (ReadToken(false, token, input)) {\n"
    "    if (*token == TOKEN_NULL) {\n"
    "      // multi char tokens are always eaten\n"
    "      continue;\n"
    "    } else if (*token == TOKEN_COMMA) {\n"
    "      if (!ReadToken(true, token, input) ||\n"
    "          *token != TOKEN_COMMA) {\n"
    "        RTN_FALSE;\n"
    "      }\n"
    "      ++*cur_field_num;\n"
    "    } else if (*token == TOKEN_SQUARE_CLOSE) {\n"
    "      *cur_field_num = -1;\n"
    "      return true;\n"
    "    } else if (*token == TOKEN_NONE) {\n"
    "      RTN_FALSE;\n"
    "    } else {\n"
    "      return true;\n"
    "    }\n"
    "  }\n"
    "  RTN_FALSE;\n"
    "}\n"
    "\n"
    "bool ReadObjectKeyName(\n"
    "    std::string *value,\n"
    "    Token *token,\n"
    "    google::protobuf::io::ZeroCopyInputStream *input) {\n"
    "  if (!ReadToken(false, token, input)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (*token == TOKEN_COMMA) {\n"
    "    if (!ReadToken(true, token, input) ||\n"
    "        !ReadToken(false, token, input)) {\n"
    "      RTN_FALSE;\n"
    "    }\n"
    "  } else if (*token == TOKEN_CURLY_CLOSE) {\n"
    "      *value = \"\";\n"
    "      return true;\n"
    "  }\n"
    "  if (!ReadToken(true, token, input) ||\n"
    "      *token != TOKEN_STRING) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!ReadString(value, input)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (value->empty()) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!ReadToken(true, token, input) || *token != TOKEN_COLON) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (!ReadToken(false, token, input)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  return true;\n"
    "}\n"
    "\n"
    "bool ReadObjectKeyTag(\n"
    "    google::protobuf::int32 *cur_field_num,\n"
    "    Token *token,\n"
    "    google::protobuf::io::ZeroCopyInputStream *input) {\n"
    "  std::string value;\n"
    "  if (!ReadObjectKeyName(&value, token, input)) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  if (value.empty()) {\n"
    "    *cur_field_num = -1;\n"
    "    return true;\n"
    "  }\n"
    "  if (sscanf(value.c_str(), \"%d\", cur_field_num) != 1) {\n"
    "    RTN_FALSE;\n"
    "  }\n"
    "  return true;\n"
    "}\n"
    "\n"
    "}  // namespace\n"
    "\n";

}  // namespace internal

CodeGenerator::CodeGenerator(const std::string &name)
    : name_(name) {}

CodeGenerator::~CodeGenerator() {}

bool CodeGenerator::HeaderFile(
    const std::string &output_h_file_name,
    const std::string &class_scope,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error) const {
  google::protobuf::internal::scoped_ptr<
    google::protobuf::io::ZeroCopyOutputStream> output_h(
        output_directory->OpenForInsert(output_h_file_name,
                                        class_scope));
  google::protobuf::io::Printer h_printer(output_h.get(), '$');
  h_printer.Print(
      "bool SerializePartialToZeroCopyJsonStream(\n"
      "    const google::protobuf::uint32 type,\n"
      "    const bool booleans_as_numbers,\n"
      "    const bool start_index_one,\n"
      "    google::protobuf::io::ZeroCopyOutputStream *output) const;\n"
      "\n"
      "bool SerializePartialToPbLiteString(std::string *output) const;\n"
      "\n"
      "bool SerializePartialToPbLiteZeroIndexString(\n"
      "    std::string *output) const;\n"
      "\n"
      "bool SerializePartialToObjectKeyNameString(\n"
      "    std::string *output) const;\n"
      "\n"
      "bool SerializePartialToObjectKeyTagString(std::string *output) const;\n"
      "\n"
      "bool ParsePartialFromZeroCopyJsonStream(\n"
      "    const google::protobuf::uint32 type,\n"
      "    const bool booleans_as_numbers,\n"
      "    const bool start_index_one,\n"
      "    google::protobuf::io::ZeroCopyInputStream *input);\n"
      "\n"
      "bool ParsePartialFromPbLiteArray(const void *data, int size);\n"
      "\n"
      "bool ParsePartialFromPbLiteZeroIndexArray(\n"
      "    const void *data, int size);\n"
      "\n"
      "bool ParsePartialFromPbLiteString(const std::string &output);\n"
      "\n"
      "bool ParsePartialFromPbLiteZeroIndexString(\n"
      "    const std::string &output);\n"
      "\n"
      "bool ParsePartialFromObjectKeyNameArray(const void *data, int size);\n"
      "\n"
      "bool ParsePartialFromObjectKeyNameString(const std::string &output);\n"
      "\n"
      "bool ParsePartialFromObjectKeyTagArray(const void *data, int size);\n"
      "\n"
      "bool ParsePartialFromObjectKeyTagString(const std::string &output);\n"
      "\n");

  if (h_printer.failed()) {
    *error = "CppJsCodeGenerator detected write error.";
    return false;
  }

  return true;
}

bool CodeGenerator::CppFileHelperFunctions(
    const std::string &output_cc_file_name,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error) const {
  // Note: These functions should really be inserted at the global_scope
  // however they need to come at the start of the file so we insert them
  // after the includes instead.
  google::protobuf::internal::scoped_ptr<
    google::protobuf::io::ZeroCopyOutputStream> output_cc(
        output_directory->OpenForInsert(output_cc_file_name, "includes"));
  google::protobuf::io::Printer cc_printer(output_cc.get(), '$');
  cc_printer.Print(internal::cc_header_boilerplate.c_str());

  if (cc_printer.failed()) {
    *error = "CppJsCodeGenerator detected write error.";
    return false;
  }

  return true;
}


bool CodeGenerator::SerializePartialToZeroCopyJsonStream(
    const std::string &output_cc_file_name,
    const google::protobuf::Descriptor *message,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error) const {
  google::protobuf::internal::scoped_ptr<
    google::protobuf::io::ZeroCopyOutputStream> output_cc(
        output_directory->OpenForInsert(output_cc_file_name,
                                        "namespace_scope"));
  google::protobuf::io::Printer cc_printer(output_cc.get(), '$');
  const std::string base = message->containing_type() ?
      message->containing_type()->full_name() + "_" : "";
  const std::string cc_class_name = base + message->name();

  cc_printer.Print(
      "bool $name$::SerializePartialToZeroCopyJsonStream(\n"
      "    const google::protobuf::uint32 type,\n"
      "    const bool booleans_as_numbers,\n"
      "    const bool start_index_one,\n"
      "    google::protobuf::io::ZeroCopyOutputStream *output) const {\n",
      "name", cc_class_name);
  cc_printer.Indent();
  if (message->field_count()) {
    cc_printer.Print(
        "google::protobuf::uint32 cur_field_num = start_index_one ? 1 : 0;\n"
        "bool prev_fields = false;\n");
  }
  cc_printer.Print(
      "if (!WriteRaw(type == PB_LITE ? \"[\" : \"{\", output)) {\n"
      "  RTN_FALSE;\n"
      "}\n");

  std::string rep_field_set =
      "std::set<google::protobuf::uint32> rep_field_set;";
  for (int j = 0; j < message->field_count(); ++j) {
    const google::protobuf::FieldDescriptor *field = message->field(j);
    if (field->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
      char field_number[13];  // ceiling(32/3) + sign char + NULL
      if (snprintf(field_number,
                   sizeof(field_number),
                   "%d",
                   field->number()) >= 13) {
        return false;
      }
      rep_field_set += ("rep_field_set.insert(" +
                        std::string(field_number) +
                        ");");
    }
  }

  for (int j = 0; j < message->field_count(); ++j) {
    const google::protobuf::FieldDescriptor *field = message->field(j);
    if (field->label() != google::protobuf::FieldDescriptor::LABEL_REPEATED) {
      cc_printer.Print("// $name$\n"
                        "if (has_$name$()) {\n",
                        "name", field->lowercase_name());
    } else {
      cc_printer.Print("// $name$\n"
                        "if (this->$name$_size() > 0) {\n",
                        "name", field->lowercase_name());
    }
    cc_printer.Indent();

    char field_number[13];  // ceiling(32/3) + sign char + NULL
    if (snprintf(
            field_number, sizeof(field_number), "%d", field->number()) >= 13) {
      return false;
    }
    cc_printer.Print(
        "if (type == PB_LITE) {\n"
        "  $rep_field_set$\n"
        "  if (!WritePbLiteNullEntries(\n"
        "      $field_num$, rep_field_set, start_index_one, &cur_field_num,\n"
        "      output)) {\n"
        "    RTN_FALSE;\n"
        "  }\n"
        "} else {\n"
        "  if (type == OBJECT_KEY_TAG) {\n"
        "    if (!WriteObjectKey(\"$field_num$\", prev_fields, output)) {\n"
        "      RTN_FALSE;\n"
        "    }\n"
        "  } else if (type == OBJECT_KEY_NAME) {\n"
        "    if (!WriteObjectKey(\"$field_name$\", prev_fields, output)) {\n"
        "      RTN_FALSE;\n"
        "    }\n"
        "  } else {\n"
        "    RTN_FALSE;\n"
        "  }\n"
        "  prev_fields = true;\n"
        "}\n",
        "rep_field_set", rep_field_set,
        "field_num", field_number,
        "field_name", field->name());

    if (field->label() ==
        google::protobuf::FieldDescriptor::LABEL_REPEATED) {
      cc_printer.Print(
          "if (!WriteRaw(\"[\", output)) {\n"
          "  RTN_FALSE;\n"
          "}\n");
    }

    if (field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL) {
      if (field->label() !=
          google::protobuf::FieldDescriptor::LABEL_REPEATED) {
        cc_printer.Print(
            "if (booleans_as_numbers) {\n"
            "  if (!WriteRaw(this->$name$() ? \"1\" : \"0\", output)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "} else {\n"
            "  if (!WriteRaw(this->$name$() ? "  // no newline
            "\"true\" : \"false\", output)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name());
      } else {
        cc_printer.Print(
            "for (int i = 0; i < this->$name$_size(); ++i) {\n"
            "  if (!WriteRaw(this->$name$(i) ? \"1\" : \"0\", output)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (i < this->$name$_size() - 1) {\n"
            "    if (!WriteRaw(\",\", output)) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name());
      }
    } else if (
        field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES ||
        field->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
      if (field->label() !=
          google::protobuf::FieldDescriptor::LABEL_REPEATED) {
        cc_printer.Print(
            "if (!WriteString(this->$name$(), output)) {\n"
            "  RTN_FALSE;\n"
            "}\n",
            "name", field->lowercase_name());
      } else {
        cc_printer.Print(
            "for (int i = 0; i < this->$name$_size(); ++i) {\n"
            "  if (!WriteString(this->$name$(i), output)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (i < this->$name$_size() - 1) {\n"
            "    if (!WriteRaw(\",\", output)) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name());
      }
    } else if (
        field->type() == google::protobuf::FieldDescriptor::TYPE_GROUP ||
        field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
      if (field->label() !=
          google::protobuf::FieldDescriptor::LABEL_REPEATED) {
        cc_printer.Print(
            "if (!this->$name$()."  // no newline
            "SerializePartialToZeroCopyJsonStream(type, "  // no newline
            "booleans_as_numbers, start_index_one, output)) {\n"
            "  RTN_FALSE;\n"
            "}\n",
            "name", field->lowercase_name());
      } else {
        cc_printer.Print(
            "for (int i = 0; i < this->$name$_size(); ++i) {\n"
            "  if (!this->$name$(i)."  // no newline
            "SerializePartialToZeroCopyJsonStream(type, "  // no newline
            "booleans_as_numbers, start_index_one, output)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (i < this->$name$_size() - 1) {\n"
            "    if (!WriteRaw(\",\", output)) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name());
      }
    } else {
      std::string format_string = "\\\"%ld\\\"";
      // ceiling(64/3) + sign char + 2 quotes + NULL;
      std::string buffer_size = "26";
      if (field->options().GetExtension(jstype)) {
        format_string = "%ld";
        // ceiling(64/3) + sign char + NULL;
        std::string buffer_size = "24";
      }
      if (field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT) {
        format_string = "%g";
      } else if (
          field->type() == google::protobuf::FieldDescriptor::TYPE_UINT64 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_FIXED64) {
        if (field->options().GetExtension(jstype)) {
          format_string = "%lu";
          buffer_size = "23";  // ceiling(64/3) + NULL
        } else {
          format_string = "\\\"%lu\\\"";
          buffer_size = "25";  // ceiling(64/3) + 2 quotes + NULL
        }
      } else if (
          field->type() == google::protobuf::FieldDescriptor::TYPE_INT32 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_SINT32 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_SFIXED32 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_ENUM) {
        format_string = "%d";
        buffer_size = "13";  // ceiling(32/3) + sign char + NULL
      } else if (
          field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_FIXED32) {
        format_string = "%u";
        buffer_size = "12";  // ceiling(32/3) + NULL
      }
      if (field->label() !=
          google::protobuf::FieldDescriptor::LABEL_REPEATED) {
        cc_printer.Print(
            "{\n"
            "  char buffer[$buffer_size$];\n"
            "  if (snprintf(buffer, sizeof(buffer), "  // no newline
            "\"$format$\", this->$name$()) >= "  // no newline
            "$buffer_size$) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (!WriteRaw(buffer, output)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name(),
            "buffer_size", buffer_size,
            "format", format_string);
      } else {
        cc_printer.Print(
            "for (int i = 0; i < this->$name$_size(); ++i) {\n"
            "  char buffer[$buffer_size$];\n"
            "  if (snprintf(buffer,\n"
            "               sizeof(buffer),\n"
            "               ""\"$format$\",\n"
            "               this->$name$(i)) >= $buffer_size$) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (!WriteRaw(buffer, output)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (i < this->$name$_size() - 1) {\n"
            "    if (!WriteRaw(\",\", output)) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name(),
            "format", format_string,
            "buffer_size", buffer_size);
      }
    }

    if (field->label() ==
        google::protobuf::FieldDescriptor::LABEL_REPEATED) {
      cc_printer.Print(
          "if (!WriteRaw(\"]\", output)) {\n"
          "  RTN_FALSE;\n"
          "}\n");
    }

    cc_printer.Outdent();
    cc_printer.Print("}\n"
                      "\n");
  }

  // TODO(ahochhaus): Serialize unknown fields.
  cc_printer.Print(
      "if (!WriteRaw(type == PB_LITE ? \"]\" : \"}\", output)) {\n"
      "  RTN_FALSE;\n"
      "}\n"
      "return true;\n");
  cc_printer.Outdent();
  cc_printer.Print(
      "}\n"
      "\n"
      "bool $name$::SerializePartialToPbLiteString(\n"
      "    std::string *output) const {\n"
      "  google::protobuf::io::StringOutputStream target(output);\n"
      "  return SerializePartialToZeroCopyJsonStream(\n"
      "      PB_LITE, true, false, &target);\n"
      "}\n"
      "\n"
      "bool $name$::SerializePartialToPbLiteZeroIndexString(\n"
      "    std::string *output) const {\n"
      "  google::protobuf::io::StringOutputStream target(output);\n"
      "  return SerializePartialToZeroCopyJsonStream(\n"
      "      PB_LITE, true, true, &target);\n"
      "}\n"
      "\n"
      "bool $name$::SerializePartialToObjectKeyNameString(\n"
      "    std::string *output) const {\n"
      "  google::protobuf::io::StringOutputStream target(output);\n"
      "  return SerializePartialToZeroCopyJsonStream(\n"
      "      OBJECT_KEY_NAME, false, false, &target);\n"
      "}\n"
      "\n"
      "bool $name$::SerializePartialToObjectKeyTagString(\n"
      "    std::string *output) const {\n"
      "  google::protobuf::io::StringOutputStream target(output);\n"
      "  return SerializePartialToZeroCopyJsonStream(\n"
      "      OBJECT_KEY_TAG, false, false, &target);\n"
      "}\n"
      "\n",
      "name", cc_class_name);

  if (cc_printer.failed()) {
    *error = "CppJsCodeGenerator detected write error.";
    return false;
  }

  return true;
}

bool CodeGenerator::ParsePartialFromZeroCopyJsonStream(
    const std::string &output_cc_file_name,
    const google::protobuf::Descriptor *message,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error) const {
  google::protobuf::internal::scoped_ptr<
    google::protobuf::io::ZeroCopyOutputStream> output_cc(
        output_directory->OpenForInsert(output_cc_file_name,
                                        "namespace_scope"));
  google::protobuf::io::Printer cc_printer(output_cc.get(), '$');
  const std::string base = message->containing_type() ?
      message->containing_type()->full_name() + "_" : "";
  const std::string cc_class_name = base + message->name();

  cc_printer.Print(
      "bool $name$::ParsePartialFromZeroCopyJsonStream(\n"
      "    const google::protobuf::uint32 type,\n"
      "    const bool booleans_as_numbers,\n"
      "    const bool start_index_one,\n"
      "    google::protobuf::io::ZeroCopyInputStream *input) {\n",
      "name", cc_class_name);
  cc_printer.Indent();
  cc_printer.Print(
      "Token token;\n"
      "if (!ReadToken(true, &token, input) ||\n"
      "    (type == PB_LITE && token != TOKEN_SQUARE_OPEN) ||\n"
      "    (type != PB_LITE && token != TOKEN_CURLY_OPEN)) {\n"
      "  RTN_FALSE;\n"
      "}\n"
      "\n"
      "google::protobuf::int32 cur_field_num = start_index_one ? 1 : 0;\n"
      "while (true) {\n"
      "  if (type == PB_LITE) {\n"
      "    if (!ReadPbLiteNextTag(&cur_field_num, &token, input)) {\n"
      "      RTN_FALSE;\n"
      "    }\n"
      "  } else if (type == OBJECT_KEY_NAME) {\n"
      "    std::string field_name;\n"
      "    if (!ReadObjectKeyName(&field_name, &token, input)) {\n"
      "      RTN_FALSE;\n"
      "    }\n");

  cc_printer.Indent();
  cc_printer.Indent();
  cc_printer.Print(
      "if (field_name.empty()) {\n"
      "  cur_field_num = -1;\n"
      "}");
  for (int i = 0; i < message->field_count(); ++i) {
    const google::protobuf::FieldDescriptor *field = message->field(i);
    char field_number[13];  // ceiling(32/3) + sign char + NULL
    if (snprintf(
            field_number, sizeof(field_number), "%d", field->number()) >= 13) {
      return false;
    }
    cc_printer.Print(
        " else if (field_name == \"$name$\") {\n"
        "  cur_field_num = $number$;\n"
        "}",
        "name", field->name(),
        "number", field_number);
  }
  cc_printer.Print(
      " else {\n"
      "  // TODO(ahochhaus): process unknown fields.\n"
      "  cur_field_num = 0;\n"
      "}\n"
      "\n");
  cc_printer.Outdent();
  cc_printer.Outdent();

  cc_printer.Print(
      "  } else if (type == OBJECT_KEY_TAG) {\n"
      "    if (!ReadObjectKeyTag(&cur_field_num, &token, input)) {\n"
      "      RTN_FALSE;\n"
      "    }\n"
      "  } else {\n"
      "    RTN_FALSE;\n"
      "  }\n"
      "  if (cur_field_num < 0) {\n"
      "    if (!ReadToken(true, &token, input) ||\n"
      "        (type == PB_LITE && token != TOKEN_SQUARE_CLOSE) ||\n"
      "        (type != PB_LITE && token != TOKEN_CURLY_CLOSE)) {\n"
      "      RTN_FALSE;\n"
      "    }\n"
      "    return true;\n"
      "  }\n");
  cc_printer.Indent();
  if (message->field_count() > 0) {
    cc_printer.Print("switch (cur_field_num) {\n");
  }
  cc_printer.Indent();

  for (int j = 0; j < message->field_count(); ++j) {
    const google::protobuf::FieldDescriptor *field = message->field(j);

    char field_number[13];  // ceiling(32/3) + sign char + NULL
    if (snprintf(
            field_number, sizeof(field_number), "%d", field->number()) >= 13) {
      return false;
    }
    cc_printer.Print("// $name$\n"
                      "case $number$:\n",
                      "number", field_number,
                      "name", field->lowercase_name());
    cc_printer.Indent();

    if (field->label() ==
        google::protobuf::FieldDescriptor::LABEL_REPEATED) {
      cc_printer.Print(
          "if (!ReadToken(true, &token, input) ||\n"
          "    token != TOKEN_SQUARE_OPEN) {\n"
          "  RTN_FALSE;\n"
          "}\n");
    }

    if (field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL) {
      if (field->label() !=
          google::protobuf::FieldDescriptor::LABEL_REPEATED) {
        cc_printer.Print(
            "if (booleans_as_numbers && token == TOKEN_NUMBER) {\n"
            "  if (!ReadToken(true, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  std::string number;\n"
            "  if (!ReadNumber(&number, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (number == \"1\") {\n"
            "    this->set_$name$(true);\n"
            "  } else if (number == \"0\") {\n"
            "    this->set_$name$(false);\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "} else if (!booleans_as_numbers && token == TOKEN_TRUE) {\n"
            "  this->set_$name$(true);\n"
            "} else if (!booleans_as_numbers && token == TOKEN_FALSE) {\n"
            "  this->set_$name$(false);\n"
            "} else {\n"
            "  RTN_FALSE;\n"
            "}\n",
            "name", field->lowercase_name());
      } else {
        cc_printer.Print(
            "while (true) {\n"
            "  if (!ReadToken(true, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (token == TOKEN_SQUARE_CLOSE) {\n"
            "    break;\n"
            "  } else if (booleans_as_numbers && token == TOKEN_NUMBER) {\n"
            "    std::string number;\n"
            "    if (!ReadNumber(&number, input)) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "    if (number == \"1\") {\n"
            "      this->add_$name$(true);\n"
            "    } else if (number == \"0\") {\n"
            "      this->add_$name$(false);\n"
            "    } else {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "  } else if (!booleans_as_numbers && token == TOKEN_TRUE) {\n"
            "    this->add_$name$(true);\n"
            "  } else if (!booleans_as_numbers && token == TOKEN_FALSE) {\n"
            "    this->add_$name$(false);\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (!ReadToken(true, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (token == TOKEN_SQUARE_CLOSE) {\n"
            "    break;\n"
            "  } else if (token == TOKEN_COMMA) {\n"
            "    continue;\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name());
      }
    } else if (
        field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES ||
        field->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
      if (field->label() !=
          google::protobuf::FieldDescriptor::LABEL_REPEATED) {
        cc_printer.Print(
            "if (!ReadToken(true, &token, input) || token != TOKEN_STRING) {\n"
            "  RTN_FALSE;\n"
            "}\n"
            "{\n"
            "  std::string value;\n"
            "  if (!ReadString(&value, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  this->set_$name$(value);\n"
            "}\n",
            "name", field->lowercase_name());
      } else {
        cc_printer.Print(
            "while (true) {\n"
            "  if (!ReadToken(true, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (token == TOKEN_SQUARE_CLOSE) {\n"
            "    break;\n"
            "  } else if (token == TOKEN_STRING) {\n"
            "    std::string value;\n"
            "    if (!ReadString(&value, input)) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "    this->add_$name$(value);\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (!ReadToken(true, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (token == TOKEN_SQUARE_CLOSE) {\n"
            "    break;\n"
            "  } else if (token == TOKEN_COMMA) {\n"
            "    continue;\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name());
      }
    } else if (
        field->type() == google::protobuf::FieldDescriptor::TYPE_GROUP ||
        field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
      if (field->label() !=
          google::protobuf::FieldDescriptor::LABEL_REPEATED) {
        cc_printer.Print(
            "if (!this->mutable_$name$()->"  // no newline
            "ParsePartialFromZeroCopyJsonStream(type, "  // no newline
            "booleans_as_numbers, start_index_one, input)) {\n"
            "  RTN_FALSE;\n"
            "}\n",
            "name", field->lowercase_name());
      } else {
        cc_printer.Print(
            "while (true) {\n"
            "  if (!ReadToken(false, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (type == PB_LITE && token == TOKEN_SQUARE_CLOSE ||\n"
            "      type != PB_LITE && token == TOKEN_CURLY_CLOSE) {\n"
            "    ReadToken(true, &token, input);\n"
            "    break;\n"
            "  } else if (type == PB_LITE && token == TOKEN_SQUARE_OPEN ||\n"
            "             type != PB_LITE && token == TOKEN_CURLY_OPEN) {\n"
            "    if (!this->add_$name$()->"  // no newline
            "ParsePartialFromZeroCopyJsonStream(type, "  // no newline
            "booleans_as_numbers, start_index_one, input)) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (!ReadToken(true, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (token == TOKEN_SQUARE_CLOSE) {\n"
            "    break;\n"
            "  } else if (token == TOKEN_COMMA) {\n"
            "    continue;\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name());
      }
    } else {
      std::string type = "google::protobuf::int64";
      std::string format_string = "%ld";
      std::string type_cast = "&value";
      std::string number_type = "FromString";
      if (field->options().GetExtension(jstype)) {
        number_type = "";
      }
      if (field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE) {
        type = "double";
        format_string = "%lg";
        number_type = "";
      } else if (
          field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT) {
        type = "float";
        format_string = "%g";
        number_type = "";
      } else if (
          field->type() == google::protobuf::FieldDescriptor::TYPE_UINT64 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_FIXED64) {
        type = "google::protobuf::uint64";
        format_string = "%lu";
      } else if (
          field->type() == google::protobuf::FieldDescriptor::TYPE_INT32 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_SINT32 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_SFIXED32) {
        type = "google::protobuf::int32";
        format_string = "%d";
        number_type = "";
      } else if (
          field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32 ||
          field->type() == google::protobuf::FieldDescriptor::TYPE_FIXED32) {
        type = "google::protobuf::uint32";
        format_string = "%u";
        number_type = "";
      } else if (
          field->type() == google::protobuf::FieldDescriptor::TYPE_ENUM) {
        type = internal::ReplaceAll(
            ".", "::", field->enum_type()->full_name());
        format_string = "%d";
        type_cast = "reinterpret_cast<google::protobuf::int32 *> (&value)";
        number_type = "";
      }

      if (field->label() !=
          google::protobuf::FieldDescriptor::LABEL_REPEATED) {
        cc_printer.Print(
            "{\n"
            "  std::string number;\n"
            "  if (!ReadNumber$number_type$(&number, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  $type$ value;\n",
            "type", type,
            "number_type", number_type);
        cc_printer.Print(
            "  if (sscanf(number.c_str(),\n"
            "             \"$format_string$\",\n"
            "             $type_cast$) != 1) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  this->set_$name$(value);\n"
            "}\n",
            "name", field->lowercase_name(),
            "format_string", format_string,
            "type_cast", type_cast);
      } else {
        cc_printer.Print(
            "while (true) {\n"
            "  if (!ReadToken(false, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (token == TOKEN_SQUARE_CLOSE) {\n"
            "    ReadToken(true, &token, input);\n"
            "    break;\n"
            "  } else if (token == TOKEN_NUMBER || token == TOKEN_STRING) {\n"
            "    std::string number;\n"
            "    if (!ReadNumber$number_type$(&number, input)) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "    $type$ value;\n",
            "type", type,
            "number_type", number_type);
        cc_printer.Print(
            "    if (sscanf(number.c_str(),\n"
            "               \"$format_string$\",\n"
            "               $type_cast$) != 1) {\n"
            "      RTN_FALSE;\n"
            "    }\n"
            "    this->add_$name$(value);\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (!ReadToken(true, &token, input)) {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "  if (token == TOKEN_SQUARE_CLOSE) {\n"
            "    break;\n"
            "  } else if (token == TOKEN_COMMA) {\n"
            "    continue;\n"
            "  } else {\n"
            "    RTN_FALSE;\n"
            "  }\n"
            "}\n",
            "name", field->lowercase_name(),
            "format_string", format_string,
            "type_cast", type_cast);
      }
    }

    cc_printer.Print("break;\n");
    cc_printer.Outdent();
    cc_printer.Print("\n");
  }

  // TODO(ahochhaus): Deserialize unknown fields.

  cc_printer.Outdent();
  if (message->field_count() > 0) {
    cc_printer.Print(
        "  default:\n"
        "    RTN_FALSE;\n"
        "    break;\n"
        "}\n");
  }
  cc_printer.Outdent();
  cc_printer.Print(
      "}\n"
      "\n");

  cc_printer.Outdent();
  cc_printer.Print(
      "  RTN_FALSE;\n"
      "}\n"
      "\n"
      "bool $name$::ParsePartialFromPbLiteArray(\n"
      "    const void *data, int size) {\n"
      "  google::protobuf::io::ArrayInputStream input(\n"
      "      reinterpret_cast<const google::protobuf::uint8 *>(data), size);\n"
      "  return ParsePartialFromZeroCopyJsonStream(\n"
      "      PB_LITE, true, false, &input);\n"
      "}\n"
      "\n"
      "bool $name$::ParsePartialFromPbLiteZeroIndexArray(\n"
      "    const void *data, int size) {\n"
      "  google::protobuf::io::ArrayInputStream input(\n"
      "      reinterpret_cast<const google::protobuf::uint8 *>(data), size);\n"
      "  return ParsePartialFromZeroCopyJsonStream(\n"
      "      PB_LITE, true, true, &input);\n"
      "}\n"
      "\n"
      "bool $name$::ParsePartialFromPbLiteString(\n"
      "    const std::string &output) {\n"
      "  return ParsePartialFromPbLiteArray(output.data(), output.size());\n"
      "}\n"
      "\n"
      "bool $name$::ParsePartialFromPbLiteZeroIndexString(\n"
      "    const std::string &output) {\n"
      "  return ParsePartialFromPbLiteZeroIndexArray(\n"
      "      output.data(), output.size());\n"
      "}\n"
      "\n"
      "bool $name$::ParsePartialFromObjectKeyNameArray(\n"
      "    const void *data, int size) {\n"
      "  google::protobuf::io::ArrayInputStream input(\n"
      "      reinterpret_cast<const google::protobuf::uint8 *>(data), size);\n"
      "  return ParsePartialFromZeroCopyJsonStream(\n"
      "      OBJECT_KEY_NAME, false, false, &input);\n"
      "}\n"
      "\n"
      "bool $name$::ParsePartialFromObjectKeyNameString(\n"
      "    const std::string &output) {\n"
      "  return ParsePartialFromObjectKeyNameArray(\n"
      "      output.data(), output.size());\n"
      "}\n"
      "\n"
      "bool $name$::ParsePartialFromObjectKeyTagArray(\n"
      "    const void *data, int size) {\n"
      "  google::protobuf::io::ArrayInputStream input(\n"
      "      reinterpret_cast<const google::protobuf::uint8 *>(data), size);\n"
      "  return ParsePartialFromZeroCopyJsonStream(\n"
      "      OBJECT_KEY_TAG, false, false, &input);\n"
      "}\n"
      "\n"
      "bool $name$::ParsePartialFromObjectKeyTagString(\n"
      "    const std::string &output) {\n"
      "  return ParsePartialFromObjectKeyTagArray(\n"
      "      output.data(), output.size());\n"
      "}\n"
      "\n",
      "name", cc_class_name);

  if (cc_printer.failed()) {
    *error = "CppJsCodeGenerator detected write error.";
    return false;
  }

  return true;
}

bool CodeGenerator::InstrumentMessage(
    const std::string &output_h_file_name,
    const std::string &output_cc_file_name,
    const google::protobuf::Descriptor *message,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error) const {
  const std::string class_scope = "class_scope:" + message->full_name();
  if (!CodeGenerator::HeaderFile(output_h_file_name,
                                 class_scope,
                                 output_directory,
                                 error)) {
    return false;
  }
  if (!CodeGenerator::SerializePartialToZeroCopyJsonStream(
          output_cc_file_name,
          message,
          output_directory,
          error)) {
    return false;
  }
  if (!CodeGenerator::ParsePartialFromZeroCopyJsonStream(
          output_cc_file_name,
          message,
          output_directory,
          error)) {
    return false;
  }

  for (int i = 0; i < message->nested_type_count(); ++i) {
    const google::protobuf::Descriptor *sub_message = message->nested_type(i);
    if (!InstrumentMessage(output_h_file_name,
                           output_cc_file_name,
                           sub_message,
                           output_directory,
                           error)) {
      return false;
    }
  }
  return true;
}


bool CodeGenerator::Generate(
    const google::protobuf::FileDescriptor *file,
    const std::string &/* parameter */,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error) const {

  std::string output_h_file_name = file->name();
  std::size_t loc = output_h_file_name.rfind(".");
  output_h_file_name.erase(loc, output_h_file_name.length() - loc);
  std::string output_cc_file_name = output_h_file_name;
  output_h_file_name.append(".pb.h");
  output_cc_file_name.append(".pb.cc");

  if (!CodeGenerator::CppFileHelperFunctions(output_cc_file_name,
                                             output_directory,
                                             error)) {
    return false;
  }

  for (int i = 0; i < file->message_type_count(); ++i) {
    const google::protobuf::Descriptor *message = file->message_type(i);

    if (!InstrumentMessage(output_h_file_name,
                           output_cc_file_name,
                           message,
                           output_directory,
                           error)) {
      return false;
    }
  }

  return true;
}

}  // namespace ccjs
}  // namespace protobuf
}  // namespace sg
