// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>  // NOLINT
#include <string.h> // NOLINT
#include <readline/readline.h> // NOLINT
#include <readline/history.h> // NOLINT

// The readline includes leaves RETURN defined which breaks V8 compilation.
#undef RETURN

#include "src/d8.h"

// There are incompatibilities between different versions and different
// implementations of readline.  This smooths out one known incompatibility.
#if RL_READLINE_VERSION >= 0x0500
#define completion_matches rl_completion_matches
#endif


namespace v8 {


class ReadLineEditor: public LineEditor {
 public:
  ReadLineEditor() : LineEditor(LineEditor::READLINE, "readline") { }
  virtual Handle<String> Prompt(const char* prompt);
  virtual bool Open(Isolate* isolate);
  virtual bool Close();
  virtual void AddHistory(const char* str);

  static const char* kHistoryFileName;
  static const int kMaxHistoryEntries;

 private:
#ifndef V8_SHARED
  static char** AttemptedCompletion(const char* text, int start, int end);
  static char* CompletionGenerator(const char* text, int state);
#endif  // V8_SHARED
  static char kWordBreakCharacters[];

  Isolate* isolate_;
};


static ReadLineEditor read_line_editor;
char ReadLineEditor::kWordBreakCharacters[] = {' ', '\t', '\n', '"',
    '\\', '\'', '`', '@', '.', '>', '<', '=', ';', '|', '&', '{', '(',
    '\0'};


const char* ReadLineEditor::kHistoryFileName = ".d8_history";
const int ReadLineEditor::kMaxHistoryEntries = 1000;


bool ReadLineEditor::Open(Isolate* isolate) {
  isolate_ = isolate;

  rl_initialize();

#ifdef V8_SHARED
  // Don't do completion on shared library mode
  // http://cnswww.cns.cwru.edu/php/chet/readline/readline.html#SEC24
  rl_bind_key('\t', rl_insert);
#else
  rl_attempted_completion_function = AttemptedCompletion;
#endif  // V8_SHARED

  rl_completer_word_break_characters = kWordBreakCharacters;
  rl_bind_key('\t', rl_complete);
  using_history();
  stifle_history(kMaxHistoryEntries);
  return read_history(kHistoryFileName) == 0;
}


bool ReadLineEditor::Close() {
  return write_history(kHistoryFileName) == 0;
}


Handle<String> ReadLineEditor::Prompt(const char* prompt) {
  char* result = NULL;
  result = readline(prompt);
  if (result == NULL) return Handle<String>();
  AddHistory(result);
  return String::NewFromUtf8(isolate_, result);
}


void ReadLineEditor::AddHistory(const char* str) {
  // Do not record empty input.
  if (strlen(str) == 0) return;
  // Remove duplicate history entry.
  history_set_pos(history_length-1);
  if (current_history()) {
    do {
      if (strcmp(current_history()->line, str) == 0) {
        remove_history(where_history());
        break;
      }
    } while (previous_history());
  }
  add_history(str);
}


#ifndef V8_SHARED
char** ReadLineEditor::AttemptedCompletion(const char* text,
                                           int start,
                                           int end) {
  char** result = completion_matches(text, CompletionGenerator);
  rl_attempted_completion_over = true;
  return result;
}


char* ReadLineEditor::CompletionGenerator(const char* text, int state) {
  static unsigned current_index;
  static Persistent<Array> current_completions;
  Isolate* isolate = read_line_editor.isolate_;
  HandleScope scope(isolate);
  Handle<Array> completions;
  if (state == 0) {
    Local<String> full_text = String::NewFromUtf8(isolate,
                                                  rl_line_buffer,
                                                  String::kNormalString,
                                                  rl_point);
    completions = Shell::GetCompletions(isolate,
                                        String::NewFromUtf8(isolate, text),
                                        full_text);
    current_completions.Reset(isolate, completions);
    current_index = 0;
  } else {
    completions = Local<Array>::New(isolate, current_completions);
  }
  if (current_index < completions->Length()) {
    Handle<Integer> index = Integer::New(isolate, current_index);
    Handle<Value> str_obj = completions->Get(index);
    current_index++;
    String::Utf8Value str(str_obj);
    return strdup(*str);
  } else {
    current_completions.Reset();
    return NULL;
  }
}
#endif  // V8_SHARED


}  // namespace v8
