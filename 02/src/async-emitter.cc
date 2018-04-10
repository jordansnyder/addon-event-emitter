/*******************************************************************************
 * Copyright (c) 2018 Nicola Del Gobbo
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the license at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY
 * IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
 * MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 *
 * Contributors - initial API implementation:
 * Nicola Del Gobbo <nicoladelgobbo@gmail.com>
 ******************************************************************************/

#include<napi.h>

#include <chrono>
#include <thread>
#include <iostream>

#include "js-emitter.cc"

class EmitterWorker : public Napi::AsyncWorker {
    public:
        EmitterWorker(const Napi::Function& callback, const Napi::Function& emitter)
            : Napi::AsyncWorker(callback), emit(Napi::Persistent(emitter)) {
                // Not use this code but wait that this PR will be landed
                // https://github.com/nodejs/node/pull/17887
                this->ec = new JSEmitter(this, OnProgress);
        }

        ~EmitterWorker() {
            // Reset the emitter function reference
            this->emit.Reset();
        }

        inline static void OnProgress (uv_async_t *async) {
            EmitterWorker *worker = static_cast<EmitterWorker*>(async->data);
            Napi::HandleScope scope(worker->_env);
            worker->emit.Call({Napi::String::New(worker->_env, "data"), Napi::Number::New(worker->_env, worker->tasks)});
        }

        void Execute() {
            // Here some long running task and return piece of data exectuing some task
            for(int i = 0; i < 3; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                this->tasks = i;
                this->ec->Exec();
            }
            this->ec->End();
        }

        /*void OnData(int progress) {
            std::cout <<  "Progress on the native side: " << progress << std::endl;  
            //emit.Call({Napi::String::New(Env(), "data"), Napi::Number::New(Env(), progress)});
            //emit.MakeCallback(Env().Global(), {Napi::String::New(Env(), "data"), Napi::Number::New(Env(), progress)});
        }*/

        void OnOK() {
            Napi::HandleScope scope(Env());
            emit.Call({ Napi::String::New(Env(), "end") });
            Callback().Call({ Napi::String::New(Env(), "OK") });
        }

    private:
        Napi::FunctionReference emit;
        int tasks = 0;
        Napi::Env _env = Env();
        JSEmitter* ec;
};

Napi::Value CallAsyncEmit(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Function emitter = info[0].As<Napi::Function>();
    Napi::Function callback = info[1].As<Napi::Function>();
    emitter.Call({ Napi::String::New(env, "start") });
    EmitterWorker* emitterWorker = new EmitterWorker(callback, emitter);
    emitterWorker->Queue();
    return env.Undefined();
}

// Init
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "callAsyncEmit"), Napi::Function::New(env, CallAsyncEmit));
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init);