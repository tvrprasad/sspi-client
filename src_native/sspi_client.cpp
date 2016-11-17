// This file has the code for initializing V8 and hooking up JavaScript objects and
// methods to corresponding C++ code.

// Public API is implemented in a JavaScript layer on top of the Native binding. All
// error checking is done in JavaScript layer. Native code expects the right type and
// number of parameters to be passed in, and semantics of invocation to be honored by
// the caller. The only error checking that'll be done in the Native layer would be
// checks that can't be done in the JavaScript layer.

// In short, native binding is meant to used by the package implementation. Application
// should only use API surfaced in JavaScript.

#include <nan.h>
#include <string>

#include "sspi_impl.h"
#include "utils.h"

// Worker class for executing SSPI initialization code asynchronously.
class SspiClientInitializeWorker : public Nan::AsyncWorker
{
public:
    SspiClientInitializeWorker(Nan::Callback* callback)
        : Nan::AsyncWorker(callback)
    {
        DebugLog("%ul: Main event loop: SspiClientInitializeWorker::SspiClientInitializeWorker.\n",
            GetCurrentThreadId());
    }

    // This function executes inside the worker-thread. No V8 data-structures
    // may be accessed safely from here. To ensure this, shift excecution to
    // a class with no V8 dependencies. Store results of execution in member
    // variables to be passed in to the callback on main event loop thread.
    void Execute()
    {
        DebugLog("%ul: Worker Thread: Initialize: SspiClientInitializeWorker::Execute.\n",
            GetCurrentThreadId());

        m_securityStatus = SspiImpl::Initialize(m_errorString, c_errorStringBufferSize);
    }

    // Executed in main event loop thread after async work is completed. Invokes
    // user callback with the results from initialization.
    void HandleOKCallback()
    {
        DebugLog("%ul: Main event loop: SspiClientInitializeWorker::HandleOKCallback.\n",
            GetCurrentThreadId());

        v8::Local<v8::Value> argv[] =
        {
            Nan::New<v8::Uint32>(m_securityStatus),
            Nan::New<v8::String>(m_errorString).ToLocalChecked()
        };

        callback->Call(2, argv);
    }

    ~SspiClientInitializeWorker()
    {
        DebugLog("%ul: Garbage Collection Thread: SspiClientInitializeWorker::~SspiClientAsyncWorker.\n",
            GetCurrentThreadId());
    }

private:
    // Not implemented.
    SspiClientInitializeWorker(const SspiClientInitializeWorker&);
    SspiClientInitializeWorker& operator=(const SspiClientInitializeWorker&);

    static const int c_errorStringBufferSize = 256;

    SECURITY_STATUS m_securityStatus = SEC_E_INTERNAL_ERROR;
    char m_errorString[c_errorStringBufferSize];
};

// Worker class to get the next client response asynchronously.
class SspiClientGetNextBlobWorker : public Nan::AsyncWorker
{
public:
    SspiClientGetNextBlobWorker(Nan::Callback* callback, const char* serverName)
        : Nan::AsyncWorker(callback), m_serverName(serverName)
    {
        DebugLog("%ul: Main event loop: SspiClientGetNextBlobWorker::SspiClientInitializeWorker. serverName=%s\n",
            GetCurrentThreadId(), m_serverName.c_str());
    }

    // This function executes inside the worker-thread. No V8 data-structures
    // may be accessed safely from here. To ensure this, shift excecution to
    // a class with no V8 dependencies. Store results of execution in member
    // variables to be passed in to the callback on main event loop thread.
    void Execute()
    {
        DebugLog("%ul: Worker Thread: Initialize: SspiClientGetNextBlobWorker::Execute.\n",
            GetCurrentThreadId());

        m_securityStatus = SspiImpl::GetNextBlob(
            m_serverName.c_str(),
            &m_blob,
            &m_blobLength,
            &m_isDone,
            m_errorString,
            c_errorStringBufferSize);
    }

    // Executed in main event loop thread after async work is completed. Invokes
    // user callback with the results from initialization.
    void HandleOKCallback()
    {
        DebugLog("%ul: Main event loop: SspiClientGetNextBlobWorker::HandleOKCallback.\n",
            GetCurrentThreadId());

        v8::Local<v8::Value> argv[] =
        {
            Nan::NewBuffer(m_blob, m_blobLength, FreeCallback, nullptr).ToLocalChecked(),
            Nan::New<v8::Boolean>(m_isDone),
            Nan::New<v8::Uint32>(m_securityStatus),
            Nan::New<v8::String>(m_errorString).ToLocalChecked()
        };

        callback->Call(4, argv);
    }

    // This matches the signature for Nan::FreeCallback to free the buffer on garbage collection.
    static void FreeCallback(char* data, void* hint)
    {
        DebugLog("%ul: Garbage Collection Thread: SspiClientGetNextBlobWorker::FreeCallback.\n",
            GetCurrentThreadId());
        SspiImpl::FreeBlob(data);
    }

    ~SspiClientGetNextBlobWorker()
    {
        DebugLog("%ul: Garbage Collection Thread: SspiClientGetNextBlobWorker::~SspiClientAsyncWorker.\n",
            GetCurrentThreadId());
    }

private:
    // Not implemented.
    SspiClientGetNextBlobWorker(const SspiClientGetNextBlobWorker&);
    SspiClientGetNextBlobWorker& operator=(const SspiClientGetNextBlobWorker&);

    std::string m_serverName;
    SECURITY_STATUS m_securityStatus;
    static const int c_errorStringBufferSize = 256;
    char m_errorString[c_errorStringBufferSize];

    // This is allocated and free'd by SspiImpl class. It's lifetime is managed
    // by the V8 garbage collector.
    char* m_blob;
    int m_blobLength;
    bool m_isDone;
};

NAN_METHOD(InitializeAsync)
{
    DebugLog("%ul: Main event loop: InitializeAsync NAN_METHOD.\n", GetCurrentThreadId());

    Nan::Callback* callback = new Nan::Callback(info[0].As<v8::Function>());
    AsyncQueueWorker(new SspiClientInitializeWorker(callback));
}

NAN_METHOD(EnableDebugLogging)
{
    DebugLog("%ul: Main event loop: EnableDebugLogging NAN_METHOD.\n", GetCurrentThreadId());
    SetDebugLogging(info[0]->BooleanValue());
}

// Native implementation of SspiClient surfaced to JavaScript.
class SspiClientObject : public Nan::ObjectWrap
{
public:
    static NAN_MODULE_INIT(Init)
    {
        DebugLog("%ul: Main event loop: SspiClientObject::Init.\n", GetCurrentThreadId());
        v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
        tpl->SetClassName(Nan::New(c_className).ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        Nan::SetPrototypeMethod(tpl, "getNextBlob", GetNextBlob);

        s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
        Nan::Set(
            target,
            Nan::New(c_className).ToLocalChecked(),
            Nan::GetFunction(tpl).ToLocalChecked());
    }

private:
    // Not implemented.
    SspiClientObject(const SspiClientGetNextBlobWorker&);
    SspiClientObject& operator=(const SspiClientGetNextBlobWorker&);

    explicit SspiClientObject(const char* serverName) : m_serverName(serverName)
    {
        DebugLog("%ul: Main event loop: SspiClientObject::SspiClientObject.\n", GetCurrentThreadId());
    }

    ~SspiClientObject()
    {
        DebugLog("%ul: Garbage Collection Thread: SspiClientObject::~SspiClientObject.\n", GetCurrentThreadId());
    }

    static NAN_METHOD(New)
    {
        if (info.IsConstructCall())
        {
            // Constructor invoked with new SspiClient().
            DebugLog("%ul: Main event loop: SspiClientObject::New IsConstructorCall.\n", GetCurrentThreadId());
            Nan::Utf8String serverName(info[0]);
            SspiClientObject* sspiClientObject = new SspiClientObject(*serverName);
            sspiClientObject->Wrap(info.This());
            info.GetReturnValue().Set(info.This());
        }
        else
        {
            // Constructor invoked with SspiClient().
            DebugLog("%ul: Main event loop: SspiClientObject::New Not IsConstructorCall.\n", GetCurrentThreadId());
            v8::Local<v8::Value> argv[1] = { info[0] };

            v8::Local<v8::Function> constructor = Nan::New(s_constructor);

            // This will trigger the New method and that invocation will go through
            // the IsConstructorCall() true code path.
            info.GetReturnValue().Set(constructor->NewInstance(info.Length(), argv));
        }
    }

    static NAN_METHOD(GetNextBlob)
    {
        DebugLog("%ul: Main event loop: SspiClientObject::GetNextBlob.\n", GetCurrentThreadId());

        Nan::Callback* callback = new Nan::Callback(info[0].As<v8::Function>());
        SspiClientObject* sspiClientObject = Nan::ObjectWrap::Unwrap<SspiClientObject>(info.Holder());
        AsyncQueueWorker(new SspiClientGetNextBlobWorker(callback, sspiClientObject->m_serverName.c_str()));
    }

    std::string m_serverName;

    static Nan::Persistent<v8::Function> s_constructor;
    static const char* c_className;
};

Nan::Persistent<v8::Function> SspiClientObject::s_constructor;
const char* SspiClientObject::c_className = "SspiClient";

NAN_MODULE_INIT(Init) {
    DebugLog("%ul: Main event loop: Init NAN_MODULE_INIT.\n", GetCurrentThreadId());

    Nan::Set(
        target,
        Nan::New<v8::String>("initialize").ToLocalChecked(),
        Nan::GetFunction(Nan::New<v8::FunctionTemplate>(InitializeAsync)).ToLocalChecked());

    Nan::Set(
        target,
        Nan::New<v8::String>("enableDebugLogging").ToLocalChecked(),
        Nan::GetFunction(Nan::New<v8::FunctionTemplate>(EnableDebugLogging)).ToLocalChecked());

    SspiClientObject::Init(target);
}

NODE_MODULE(SspiClientNative, Init)
