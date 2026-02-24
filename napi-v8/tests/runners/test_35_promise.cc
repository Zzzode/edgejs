#include <string>

#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test35Promise : public FixtureTestBase {};

TEST_F(Test35Promise, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value global = nullptr;
  ASSERT_EQ(napi_get_global(s.env, &global), napi_ok);
  ASSERT_EQ(napi_set_named_property(s.env, global, "__tp", exports), napi_ok);

  auto run_js = [&](const char* source_text) {
    v8::TryCatch tc(s.isolate);
    std::string wrapped = std::string("(() => { 'use strict'; ") + source_text + " })();";
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(s.isolate, wrapped.c_str(), v8::NewStringType::kNormal)
            .ToLocalChecked();
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(s.context, source).ToLocal(&script)) return false;
    v8::Local<v8::Value> out;
    if (!script->Run(s.context).ToLocal(&out)) {
      if (tc.HasCaught()) {
        v8::String::Utf8Value msg(s.isolate, tc.Exception());
        ADD_FAILURE() << "JS exception: " << (*msg ? *msg : "<empty>")
                      << " while running: " << source_text;
      }
      return false;
    }
    s.isolate->PerformMicrotaskCheckpoint();
    return true;
  };

  ASSERT_TRUE(run_js(R"JS(
globalThis.__promiseState = {
  resolved: false,
  rejected: false,
  chained: false,
  rejectReasonMatched: false,
};
{
  const expected_result = 42;
  const promise = __tp.createPromise();
  promise.then((result) => {
    globalThis.__promiseState.resolved = (result === expected_result);
  }, () => {
    globalThis.__promiseState.resolved = false;
  });
  __tp.concludeCurrentPromise(expected_result, true);
}
{
  const expected_result = "It's not you, it's me.";
  const promise = __tp.createPromise();
  promise.then(() => {
    globalThis.__promiseState.rejected = false;
  }, (result) => {
    globalThis.__promiseState.rejected = (result === expected_result);
  });
  __tp.concludeCurrentPromise(expected_result, false);
}
{
  const promise = __tp.createPromise();
  promise.then((result) => {
    globalThis.__promiseState.chained = (result === 'chained answer');
  }, () => {
    globalThis.__promiseState.chained = false;
  });
  __tp.concludeCurrentPromise(Promise.resolve('chained answer'), true);
}
{
  const p = __tp.createPromise();
  if (__tp.isPromise(p) !== true) throw new Error('isPromiseCreated');
  __tp.concludeCurrentPromise(undefined, true);
}
{
  const rejectPromise = Promise.reject(-1);
  if (__tp.isPromise(rejectPromise) !== true) throw new Error('isPromiseNative');
  rejectPromise.catch((reason) => {
    globalThis.__promiseState.rejectReasonMatched = (reason === -1);
  });
}
if (__tp.isPromise(2.4) !== false) throw new Error('isPromiseNumber');
if (__tp.isPromise('I promise!') !== false) throw new Error('isPromiseString');
if (__tp.isPromise(undefined) !== false) throw new Error('isPromiseUndef');
if (__tp.isPromise(null) !== false) throw new Error('isPromiseNull');
if (__tp.isPromise({}) !== false) throw new Error('isPromiseObject');
)JS"));

  ASSERT_TRUE(run_js(R"JS(
if (!globalThis.__promiseState.resolved) throw new Error('resolvedCheck');
if (!globalThis.__promiseState.rejected) throw new Error('rejectedCheck');
if (!globalThis.__promiseState.chained) throw new Error('chainedCheck');
if (!globalThis.__promiseState.rejectReasonMatched) throw new Error('rejectReasonCheck');
)JS"));
}
