// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <assert.h>
#include <v8.h>

#include "unwrap.h"
#include "callback.h"

namespace uvjs {
namespace detail {

// object wraps a js handle for incrementing/decrementing the weak state
// this is used when we need an object to live for a callback
template <typename handle_t>
class HandleWrap {
public:
    HandleWrap() : _handle(0), _refs(0) {
        _handle = new handle_t();
        _handle->data = this;
    }

    virtual ~HandleWrap() {
        if (_handle) {
            delete _handle;
            _handle = NULL;
        }

        // we don't do anything with the persistent handle stuff
        // because v8 might be dead already and our After_close cb never got triggered
    }

    Callback& close_callback() {
        return _close_cb;
    }

    void close() {
        // we have a close callback so we need to stay alive for that
        if (!_close_cb.IsEmpty()) {
            this->Ref();
        }

        uv_close(_handle, After_close);
    }


    inline v8::Local<v8::Object> handle() {
        return handle(v8::Isolate::GetCurrent());
    }


    inline v8::Local<v8::Object> handle(v8::Isolate* isolate) {
        return v8::Local<v8::Object>::New(isolate, persistent());
    }


    inline v8::Persistent<v8::Object>& persistent() {
        return _obj_handle;
    }


    inline void Wrap(v8::Handle<v8::Object> handle) {
        assert(persistent().IsEmpty());
        assert(handle->InternalFieldCount() > 0);
        handle->SetAlignedPointerInInternalField(0, this);
        persistent().Reset(v8::Isolate::GetCurrent(), handle);
        MakeWeak();
    }

    /* Ref() marks the object as being attached to an event loop.
     * Refed objects will not be garbage collected, even if
     * all references are lost.
     */
    virtual void Ref() {
        assert(!persistent().IsEmpty());
        persistent().ClearWeak();
        _refs++;
    }

    /* Unref() marks an object as detached from the event loop.  This is its
     * default state.  When an object with a "weak" reference changes from
     * attached to detached state it will be freed. Be careful not to access
     * the object after making this call as it might be gone!
     * (A "weak reference" means an object that only has a
     * persistant handle.)
     *
     * DO NOT CALL THIS FROM DESTRUCTOR
     */
    virtual void Unref() {
        assert(!persistent().IsEmpty());
        assert(!persistent().IsWeak());
        assert(_refs > 0);
        if (--_refs == 0) {
            MakeWeak();
        }
    }

    static void Handle_Close(const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::HandleScope handle_scope(args.GetIsolate());

        assert(args.Length() == 1);
        assert(args[0]->IsFunction());

        HandleWrap<uv_handle_t>* wrap = Unwrap<HandleWrap<uv_handle_t> >(args.This());

        wrap->close_callback().Reset(args[0]);
        wrap->close();
    }

    static void After_close(uv_handle_t* handle) {
        v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
        HandleWrap<handle_t>* wrap = static_cast<HandleWrap<handle_t>* >(handle->data);

        if (!wrap->close_callback().IsEmpty()) {
            wrap->Unref();

            const int argc = 0;
            v8::Local<v8::Value> argv[argc] = {};
            wrap->close_callback().Call(argc, argv);
        }

        delete wrap;
    }

    static void Mixin(v8::Handle<v8::ObjectTemplate> obj) {
        obj->Set(v8::String::NewSymbol("close"), v8::FunctionTemplate::New(Handle_Close));
    }

protected:
    inline void MakeWeak(void) {
        persistent().SetWeak(this, WeakCallback);
        persistent().MarkIndependent();
    }

    handle_t* _handle;
    Callback _close_cb;

private:
    static void WeakCallback(const v8::WeakCallbackData<v8::Object, HandleWrap<handle_t> >& data) {
        v8::HandleScope scope(data.GetIsolate());

        HandleWrap<handle_t>* wrap = data.GetParameter();

        assert(wrap->_refs == 0);
        assert(wrap->_handle);

        // dragons
        // instead of clearing persistent in the destructor, we need to do it here
        // this is because v8 might be shutting down and will be dead before our
        // After_close callback has a chance to run
        wrap->persistent().ClearWeak();
        wrap->persistent().Reset();

        uv_close(reinterpret_cast<uv_handle_t*>(wrap->_handle), After_close);
    }

    int _refs;
    v8::Persistent<v8::Object> _obj_handle;
};

} // namespace detail
} // namespace uvjs

