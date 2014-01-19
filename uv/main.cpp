#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <v8.h>
#include <uv.h>

#include <string>
#include <fstream>

#include <uvjs.h>

#include "Allocator.h"
#include "natives.h"

v8::Handle<v8::Context> CreateShellContext(v8::Isolate* isolate, int argc, char* argv[]);
bool ExecuteString(v8::Handle<v8::String> source, v8::Handle<v8::Value> name);
void Print(const v8::FunctionCallbackInfo<v8::Value>& args);
void Read(const v8::FunctionCallbackInfo<v8::Value>& args);
void RunInThisContext(const v8::FunctionCallbackInfo<v8::Value>& args);
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);
v8::Handle<v8::String> ReadFile(const char* name);
void ReportException(v8::Isolate* isolate, v8::TryCatch* handler);

int main(int argc, char* argv[]) {

    Allocator allocator;

    // needed to use array buffers
    v8::V8::SetArrayBufferAllocator(&allocator);

    // or could just use v8::ArrayBuffer::Allocator
    // and do the fancy management stuff internally?
    // no, we need a non-stupid way to get at the array buffer contents
    uvjs::SetArrayBufferAllocator(&allocator);

    v8::V8::InitializeICU();
    v8::V8::SetFlagsFromCommandLine(&argc, argv, true);

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    int result = 0;
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

        v8::TryCatch try_catch;

        v8::Handle<v8::Script> script = v8::Script::Compile(
                v8::String::New(uv::bootstrap_native),
                v8::String::New("bootstrap.js"));
        script->Run();

        if (try_catch.HasCaught()) {
            ReportException(isolate, &try_catch);
            result = 1;
        }

        context->Exit();
    }

    // force garbage collection on exit
    while(!v8::V8::IdleNotification()) {};

    v8::V8::Dispose();
    return result;
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}

// Creates a new execution environment containing the built-in functions.
v8::Handle<v8::Context> CreateShellContext(v8::Isolate* isolate, int argc, char* argv[]) {
    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

    global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));
    global->Set(v8::String::New("read"), v8::FunctionTemplate::New(Read));
    global->Set(v8::String::New("run"), v8::FunctionTemplate::New(RunInThisContext));
    global->Set(v8::String::New("quit"), v8::FunctionTemplate::New(Quit));

    // expose uv into global namespace
    global->Set(v8::String::New("__uv_bindings"), uvjs::New());

    return v8::Context::New(isolate, NULL, global);
}

void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    for (int i = 0; i < args.Length(); i++) {
        if (i > 0) {
            printf(" ");
        }

        v8::String::Utf8Value str(args[i]);
        const char* cstr = ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
}


void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
    assert(args.Length() == 1);
    assert(args[0]->IsString());

    v8::String::Utf8Value file(args[0]);
    if (*file == NULL) {
        args.GetIsolate()->ThrowException(v8::String::New("Error loading file"));
        return;
    }

    v8::Handle<v8::String> source = ReadFile(*file);
    if (source.IsEmpty()) {
        std::string err = "Error loading file: ";
        err.append(*file);

        args.GetIsolate()->ThrowException(v8::String::New(err.c_str()));
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

    if (try_catch.HasCaught()) {
        ReportException(args.GetIsolate(), &try_catch);
    }

    args.GetReturnValue().Set(result);
}

void Quit(const v8::FunctionCallbackInfo<v8::Value>& args) {
    assert(args.Length() == 1);
    // If not arguments are given args[0] will yield undefined which
    // converts to the integer value 0.
    int exit_code = args[0]->Int32Value();
    fflush(stdout);
    fflush(stderr);
    exit(exit_code);
}

// Reads a file into a v8 string.
v8::Handle<v8::String> ReadFile(const char* name) {
    std::string src;
    std::string line;
    std::ifstream file(name);

    while(std::getline(file, line)) {
        src.append(line).append("\n");
    }

    return v8::String::New(src.c_str());
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Handle<v8::String> source, v8::Handle<v8::Value> name) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch;

    v8::Handle<v8::Script> script = v8::Script::Compile(source, name);

    if (script.IsEmpty()) {
        ReportException(isolate, &try_catch);
        return false;
    }

    script->Run();

    if (try_catch.HasCaught()) {
        ReportException(isolate, &try_catch);
    }

    assert(!try_catch.HasCaught());
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
        return;
    }

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
