#pragma once

#include <v8.h>
#include <uvjs.h>

// baton keeps memory alive for the duration of the array buffer
class ArrayWatchdog {
public:
    ArrayWatchdog(v8::Local<v8::ArrayBuffer>& ab, void* data)
        : _data(data) {
        _array_buffer.Reset(v8::Isolate::GetCurrent(), ab);
        _array_buffer.SetWeak(this, WeakCallback);
    }

    ~ArrayWatchdog() {
        assert(_data);
        free(_data);
        _data = NULL;
    }

    void* data() {
        return _data;
    }

private:

    static void WeakCallback(const v8::WeakCallbackData<v8::ArrayBuffer, ArrayWatchdog>& data) {
        v8::HandleScope scope(data.GetIsolate());

        ArrayWatchdog* baton = data.GetParameter();
        baton->_array_buffer.ClearWeak();
        baton->_array_buffer.Reset();
        data.GetValue()->SetAlignedPointerInInternalField(0, NULL);
        delete baton;
    }

    void* _data;
    v8::Persistent<v8::ArrayBuffer> _array_buffer;
};

class Allocator : public uvjs::ArrayBufferAllocator {
public:
    const static int kExternalField = 0;

    void* Allocate(size_t len) {
        return calloc(len, 1);
    }

    void* AllocateUninitialized(size_t len) {
        return malloc(len);
    }

    void Free(void* data, size_t len) {
        free(data);
    }

    void* Externalized(v8::Local<v8::ArrayBuffer>& buffer) {
        // buffer was already externalized
        if (buffer->IsExternal()) {
            ArrayWatchdog* baton = static_cast<ArrayWatchdog*>(buffer->GetAlignedPointerFromInternalField(kExternalField));
            return baton->data();
        }

        v8::ArrayBuffer::Contents contents = buffer->Externalize();

        ArrayWatchdog* baton = new ArrayWatchdog(buffer, contents.Data());
        buffer->SetAlignedPointerInInternalField(kExternalField, baton);

        return baton->data();
    }

    v8::Local<v8::ArrayBuffer> Externalize(void* buf, size_t bytes) {
        v8::HandleScope scope(v8::Isolate::GetCurrent());

        v8::Local<v8::ArrayBuffer> arr = v8::ArrayBuffer::New(buf, bytes);

        ArrayWatchdog* baton = new ArrayWatchdog(arr, buf);
        arr->SetAlignedPointerInInternalField(kExternalField, baton);

        return scope.Close(arr);
    }

};

