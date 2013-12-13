#pragma once

#include <assert.h>
#include <v8.h>
#include <uv.h>

#include "handle_wrap.h"
#include "callback.h"

// interface to externalize an array buffer
// it is your job to keep the array buffer alive for as long as you need

class Baton {
public:
    Baton(v8::Local<v8::ArrayBuffer>& ab, void* data)
        : _data(data) {
        _array_buffer.Reset(v8::Isolate::GetCurrent(), ab);
        _array_buffer.SetWeak(this, WeakCallback);
    }

    ~Baton() {
        assert(_data);
        free(_data);
        _data = NULL;
    }

    void* data() {
        return _data;
    }

private:

    static void WeakCallback(const v8::WeakCallbackData<v8::ArrayBuffer, Baton>& data) {
        v8::HandleScope scope(data.GetIsolate());

        Baton* baton = data.GetParameter();
        baton->_array_buffer.ClearWeak();
        baton->_array_buffer.Reset();
        data.GetValue()->SetAlignedPointerInInternalField(0, NULL);
        delete baton;
    }

    void* _data;
    v8::Persistent<v8::ArrayBuffer> _array_buffer;
};

void* Externalized(v8::Local<v8::ArrayBuffer>& buffer) {

    static int kExternalField;

    // buffer was already externalized
    if (buffer->IsExternal()) {
        Baton* baton = static_cast<Baton*>(buffer->GetAlignedPointerFromInternalField(kExternalField));
        return baton->data();
    }

    v8::ArrayBuffer::Contents contents = buffer->Externalize();

    Baton* baton = new Baton(buffer, contents.Data());
    buffer->SetAlignedPointerInInternalField(kExternalField, baton);

    return baton->data();
}

v8::Local<v8::ArrayBuffer> Externalize(void* buf, size_t bytes) {
    v8::HandleScope scope(v8::Isolate::GetCurrent());

    v8::Local<v8::ArrayBuffer> arr = v8::ArrayBuffer::New(buf, bytes);

    Baton* baton = new Baton(arr, buf);
    arr->SetAlignedPointerInInternalField(0, baton);

    return scope.Close(arr);
}

// interface to get contents of externalized buffer

namespace uvjs {
namespace detail {

template <typename T>
class StreamWrap : public HandleWrap<T> {
public:
    StreamWrap() : HandleWrap<T>() {}

    int listen(int backlog) {
        assert(this->_handle);
        this->Ref();
        return uv_listen(reinterpret_cast<uv_stream_t*>(this->_handle), backlog, After_Listen);
    }

    Callback& listen_callback() {
        return _listen_cb;
    }

    Callback& read_callback() {
        return _read_cb;
    }

    int read_start() {
        this->Ref();
        return uv_read_start(this->_handle, Alloc_Cb, Read_Cb);
    }

    int write(uv_write_t* req, uv_buf_t bufs[], const int num_bufs) {
        this->Ref();
        return uv_write(req, this->_handle, bufs, num_bufs, Write_Cb);
    }

    static void Alloc_Cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        // create an array buffer on the newly allocated data
        buf->base = static_cast<char*>(malloc(suggested_size));
        buf->len = suggested_size;

        assert(buf->base);
    }

    static void Read_Cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope handle_scope(isolate);

        assert(buf->base);
        StreamWrap<uv_stream_t>* wrap = static_cast<StreamWrap<uv_stream_t> *>(stream->data);

        // eof, buffer should be freed
        if (nread == UV_EOF) {
            free(buf->base);

            // callback with null for data to indicate to user EOF
            const int argc = 2;
            v8::Local<v8::Value> argv[argc] = { v8::Undefined() , v8::Undefined() };
            wrap->read_callback().Call(argc, argv);

            return;
        }
        // read error, buffer should be freed
        else if (nread < 0) {
            free(buf->base);

            v8::Local<v8::Value> err = v8::Exception::Error(v8::String::New("read error"));

            const int argc = 2;
            v8::Local<v8::Value> argv[argc] = { err , v8::Undefined() };
            wrap->read_callback().Call(argc, argv);
            return;
        }

        v8::Local<v8::ArrayBuffer> arr = Externalize(buf->base, nread);

        const int argc = 2;
        v8::Local<v8::Value> argv[argc] = { v8::Undefined() , arr };
        wrap->read_callback().Call(argc, argv);
    }

    static void Write_Cb(uv_write_t* req, int status);
    static void After_Listen(uv_stream_t* server, int status);

    static void Stream_Listen(const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::HandleScope handle_scope(args.GetIsolate());

        assert(args.Length() == 2);
        assert(args[0]->IsInt32());

        StreamWrap<uv_stream_t>* wrap = Unwrap<StreamWrap<uv_stream_t> >(args.This());

        wrap->listen_callback().Reset(args[1]);

        const int err = wrap->listen(args[0]->Int32Value());
        args.GetReturnValue().Set(v8::Integer::New(err));
    }

    static void Stream_Read_Start(const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::HandleScope handle_scope(args.GetIsolate());

        assert(args.Length() == 1);
        assert(args[0]->IsFunction());

        StreamWrap<uv_stream_t>* wrap = Unwrap<StreamWrap<uv_stream_t> >(args.This());

        wrap->read_callback().Reset(args[0]);
        const int err = wrap->read_start();

        args.GetReturnValue().Set(v8::Integer::New(err));
    }

    static void Stream_Write(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void Mixin(v8::Handle<v8::ObjectTemplate> obj) {
        HandleWrap<T>::Mixin(obj);

        obj->Set(v8::String::NewSymbol("listen"), v8::FunctionTemplate::New(Stream_Listen));
        obj->Set(v8::String::NewSymbol("read_start"), v8::FunctionTemplate::New(Stream_Read_Start));
        obj->Set(v8::String::NewSymbol("write"), v8::FunctionTemplate::New(Stream_Write));
    }

protected:
    Callback _listen_cb;
    Callback _read_cb;
};

class WriteReq {
public:
    WriteReq(StreamWrap<uv_stream_t>* wrap, v8::Local<v8::ArrayBuffer>& arr) {
        _wrap = wrap;
        _array_handle.Reset(v8::Isolate::GetCurrent(), arr);
        _wrap->Ref();
    }

    ~WriteReq() {
        _wrap->Unref();
        _array_handle.Reset();
    }

    Callback& write_callback() {
        return _write_cb;
    }

private:
    Callback _write_cb;
    v8::Persistent<v8::ArrayBuffer> _array_handle;
    StreamWrap<uv_stream_t>* _wrap;
};

template <typename T>
void StreamWrap<T>::Stream_Write(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 2);
    //assert(args[0]->IsArray());
    assert(args[0]->IsArrayBuffer());
    assert(args[1]->IsFunction());

    StreamWrap<uv_stream_t>* wrap = Unwrap<StreamWrap<uv_stream_t> >(args.This());

    uv_write_t* req = new uv_write_t;

    const unsigned int num_bufs = 1;
    uv_buf_t bufs[num_bufs];

    v8::Local<v8::ArrayBuffer> ab = v8::Local<v8::ArrayBuffer>::Cast(args[0]);
    bufs[0].base = static_cast<char*>(Externalized(ab));
    bufs[0].len = ab->ByteLength();

    WriteReq* write_req = new WriteReq(wrap, ab);
    write_req->write_callback().Reset(args[1]);

    // actually, req data needs to be a additional wrapper
    // which will hold the callback for this write
    req->data = write_req;

    const int err = wrap->write(req, bufs, num_bufs);

    if (err) {
        delete write_req;
        delete req;
    }

    args.GetReturnValue().Set(v8::Integer::New(err));

#if 0
    // array of ArrayBuffers
    // we can queue more than one write array at a time
    // TODO handle faster case where user just passes a single array
    // avoids additional casts
    v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(args[0]);

    // if we only send back one array this will be faster

    const unsigned int num_bufs = arr->Length();
    uv_buf_t bufs[num_bufs];

    for (unsigned int i=0 ; i<num_bufs ; ++i) {
        v8::Local<v8::Value> val = arr->Get(i);
        assert(val->IsArrayBuffer());

        v8::Local<v8::ArrayBuffer> ab = v8::Local<v8::ArrayBuffer>::Cast(val);

        bufs[i].base = static_cast<char*>(Externalized(ab));
        bufs[i].len = ab->ByteLength();
    }

#endif
}

template <typename T>
void StreamWrap<T>::Write_Cb(uv_write_t* req, int status) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handle_scope(isolate);

    WriteReq* write_req = static_cast<WriteReq*>(req->data);
    delete req;

    if (!write_req->write_callback().IsEmpty()) {
        const int argc = 1;
        v8::Local<v8::Value> argv[argc] = { v8::Integer::New(status) };
        write_req->write_callback().Call(argc, argv);
    }

    delete write_req;
}

template <typename T>
void StreamWrap<T>::After_Listen(uv_stream_t* server, int status) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);

    assert(server->data);

    StreamWrap<uv_stream_t>* wrap = static_cast<StreamWrap<uv_stream_t> *>(server->data);

    // unref here is not appropriate...
    // we can only unref when closed

    if (!wrap->listen_callback().IsEmpty()) {
        const int argc = 1;
        v8::Local<v8::Value> argv[argc] = { v8::Integer::New(status) };
        wrap->listen_callback().Call(argc, argv);
    }
}

} // namespace detail
} // namespace uvjs
