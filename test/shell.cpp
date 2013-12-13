#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <v8.h>
#include <uv.h>

#include "uvjs.h"

v8::Handle<v8::Context> CreateShellContext(v8::Isolate* isolate, int argc, char* argv[]);
void RunShell(v8::Handle<v8::Context> context);
int RunMain(v8::Isolate* isolate, int argc, char* argv[]);
bool ExecuteString(v8::Isolate* isolate,
                   v8::Handle<v8::String> source,
                   v8::Handle<v8::Value> name,
                   bool print_result,
                   bool report_exceptions);
void Print(const v8::FunctionCallbackInfo<v8::Value>& args);
void Read(const v8::FunctionCallbackInfo<v8::Value>& args);
void RunInThisContext(const v8::FunctionCallbackInfo<v8::Value>& args);
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);

v8::Handle<v8::String> ReadFile(const char* name);

void ReportException(v8::Isolate* isolate, v8::TryCatch* handler);

class Allocator : public v8::ArrayBuffer::Allocator {
public:
  Allocator() {}

  void* Allocate(size_t len) {
    return calloc(len, 1);
  }

  void* AllocateUninitialized(size_t len) {
    return malloc(len);
  }

  void Free(void* data, size_t len) {
    free(data);
  }
};

static bool run_shell;

int main(int argc, char* argv[]) {

  Allocator allocator;

  // needed to use array buffers
  v8::V8::SetArrayBufferAllocator(&allocator);

  v8::V8::InitializeICU();
  v8::V8::SetFlagsFromCommandLine(&argc, argv, true);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  run_shell = (argc == 1);
  int result;
  {
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Context> context = CreateShellContext(isolate, argc, argv);

    if (context.IsEmpty()) {
      fprintf(stderr, "Error creating context\n");
      return 1;
    }
    context->Enter();

    v8::Local<v8::Array> names = v8::Array::New(argc);
    for (int i = 0; i < argc; ++i) {
      v8::Local<v8::String> name = v8::String::NewFromUtf8(isolate, argv[i]);
      names->Set(i, name);
    }
    context->Global()->Set(v8::String::New("argv"), names);

    char cwd[PATH_MAX];
    assert(uv_cwd(cwd, PATH_MAX) == 0);
    context->Global()->Set(v8::String::New("cwd"), v8::String::NewFromUtf8(isolate, cwd));

    // run the bootstrap script
    // the bootstrap script should parse the args and do what else is needed
    // avoids having to parse crazy arg shit in c

    result = RunMain(isolate, argc, argv);
    if (run_shell) RunShell(context);

    // force garbage collection on exit
    while(!v8::V8::IdleNotification()) {};

    context->Exit();
  }

  v8::V8::Dispose();
  return result;
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

// Creates a new execution environment containing the built-in
// functions.
v8::Handle<v8::Context> CreateShellContext(v8::Isolate* isolate, int argc, char* argv[]) {
  // Create a template for the global object.
  v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

  // Bind the global 'print' function to the C++ Print callback.
  global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));

  // read a file
  global->Set(v8::String::New("read"), v8::FunctionTemplate::New(Read));

  // run the js source and return the result
  global->Set(v8::String::New("run"), v8::FunctionTemplate::New(RunInThisContext));

  // reading the Load file should happen relative to entry point file

  global->Set(v8::String::New("quit"), v8::FunctionTemplate::New(Quit));

  // expose uv into global namespace so we can test with it
  global->Set(v8::String::New("__uv_bindings"), uvjs::New());

  return v8::Context::New(isolate, NULL, global);
}


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    if (first) {
      first = false;
    } else {
      printf(" ");
    }
    v8::String::Utf8Value str(args[i]);
    const char* cstr = ToCString(str);
    printf("%s", cstr);
  }
  printf("\n");
  fflush(stdout);
}


// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1) {
    args.GetIsolate()->ThrowException(
        v8::String::New("Bad parameters"));
    return;
  }
  v8::String::Utf8Value file(args[0]);
  if (*file == NULL) {
    args.GetIsolate()->ThrowException(
        v8::String::New("Error loading file"));
    return;
  }
  v8::Handle<v8::String> source = ReadFile(*file);
  if (source.IsEmpty()) {
    args.GetIsolate()->ThrowException(
        v8::String::New("Error loading file"));
    return;
  }
  args.GetReturnValue().Set(source);
}

void RunInThisContext(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    v8::TryCatch try_catch;

    if (args.Length() == 3) {
      v8::Handle<v8::ObjectTemplate> tmpl = v8::ObjectTemplate::New();
      v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(args[2]);
      tmpl->Set(v8::String::New("global"), obj);
      tmpl->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));

      v8::Handle<v8::Context> ctx = v8::Context::New(args.GetIsolate(), NULL, tmpl);
      ctx->Enter();
    }

    v8::Handle<v8::Script> script = v8::Script::Compile(args[0]->ToString(), args[1]);

    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
      assert(try_catch.HasCaught());
      // Print errors that happened during execution.
      ReportException(args.GetIsolate(), &try_catch);
      return;
    }

    assert(!try_catch.HasCaught());
    args.GetReturnValue().Set(result);
}

// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args) {
  // If not arguments are given args[0] will yield undefined which
  // converts to the integer value 0.
  int exit_code = args[0]->Int32Value();
  fflush(stdout);
  fflush(stderr);
  exit(exit_code);
}

// Reads a file into a v8 string.
v8::Handle<v8::String> ReadFile(const char* name) {
  FILE* file = fopen(name, "rb");
  if (file == NULL) return v8::Handle<v8::String>();

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = static_cast<int>(fread(&chars[i], 1, size - i, file));
    i += read;
  }
  fclose(file);

  v8::Handle<v8::String> result = v8::String::New(chars, size);
  delete[] chars;
  return result;
}


// Process remaining command line arguments and execute files
int RunMain(v8::Isolate* isolate, int argc, char* argv[]) {
  for (int i = 1; i < argc; i++) {
    const char* str = argv[i];
    if (strcmp(str, "--shell") == 0) {
      run_shell = true;
    } else if (strcmp(str, "--bootstrap") == 0 && i + 1 < argc) {
      const char* file = argv[++i];
      v8::Handle<v8::String> source = ReadFile(file);
      v8::Handle<v8::String> file_name = v8::String::New(str);
      if (source.IsEmpty()) {
        fprintf(stderr, "Error reading '%s'\n", str);
        continue;
      }
      if (!ExecuteString(isolate, source, file_name, false, true)) return 1;
    } else if (strcmp(str, "-f") == 0) {
      // Ignore any -f flags for compatibility with the other stand-
      // alone JavaScript engines.
      continue;
    } else if (strncmp(str, "--", 2) == 0) {
      fprintf(stderr,
              "Warning: unknown flag %s.\nTry --help for options\n", str);
    } else if (strcmp(str, "-e") == 0 && i + 1 < argc) {
      // Execute argument given to -e option directly.
      v8::Handle<v8::String> file_name = v8::String::New("unnamed");
      v8::Handle<v8::String> source = v8::String::New(argv[++i]);
      if (!ExecuteString(isolate, source, file_name, false, true)) return 1;
    }
  }
  return 0;
}


// The read-eval-execute loop of the shell.
void RunShell(v8::Handle<v8::Context> context) {
  fprintf(stderr, "V8 version %s [sample shell]\n", v8::V8::GetVersion());
  static const int kBufferSize = 256;
  // Enter the execution environment before evaluating any code.
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> name(v8::String::New("(shell)"));
  while (true) {
    char buffer[kBufferSize];
    fprintf(stderr, "> ");
    char* str = fgets(buffer, kBufferSize, stdin);
    if (str == NULL) break;
    v8::HandleScope handle_scope(context->GetIsolate());
    ExecuteString(context->GetIsolate(),
                  v8::String::New(str),
                  name,
                  true,
                  true);
  }
  fprintf(stderr, "\n");
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Isolate* isolate,
                   v8::Handle<v8::String> source,
                   v8::Handle<v8::Value> name,
                   bool print_result,
                   bool report_exceptions) {
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch;

  v8::Handle<v8::Script> script = v8::Script::Compile(source, name);

  if (script.IsEmpty()) {
    // Print errors that happened during compilation.
    if (report_exceptions)
      ReportException(isolate, &try_catch);
    return false;
  }

  v8::Handle<v8::Value> result = script->Run();
  if (result.IsEmpty()) {
    assert(try_catch.HasCaught());
    // Print errors that happened during execution.
    if (report_exceptions)
      ReportException(isolate, &try_catch);
    return false;
  }

  assert(!try_catch.HasCaught());
  if (print_result && !result->IsUndefined()) {
    // If all went well and the result wasn't undefined then print
    // the returned value.
    v8::String::Utf8Value str(result);
    const char* cstr = ToCString(str);
    printf("%s\n", cstr);
  }

  return true;
}

void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
  v8::HandleScope handle_scope(isolate);
  v8::String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = ToCString(exception);
  v8::Handle<v8::Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    fprintf(stderr, "%s\n", exception_string);
  } else {
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber();
    fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
    // Print line of source code.
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = ToCString(sourceline);
    fprintf(stderr, "%s\n", sourceline_string);
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn();
    for (int i = 0; i < start; i++) {
      fprintf(stderr, " ");
    }
    int end = message->GetEndColumn();
    for (int i = start; i < end; i++) {
      fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");
    v8::String::Utf8Value stack_trace(try_catch->StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = ToCString(stack_trace);
      fprintf(stderr, "%s\n", stack_trace_string);
    }
  }
}
