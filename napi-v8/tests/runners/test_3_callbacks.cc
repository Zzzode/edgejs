#include <string>
#include <vector>

#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test3 : public FixtureTestBase {};

napi_value VerifyHello(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  void* data = nullptr;
  if (napi_get_cb_info(env, info, &argc, args, nullptr, &data) != napi_ok) return nullptr;
  auto* saw = static_cast<bool*>(data);
  char buf[64];
  if (napi_get_value_string_utf8(env, args[0], buf, sizeof(buf), nullptr) != napi_ok) return nullptr;
  *saw = (std::string(buf) == "hello world");
  return nullptr;
}

TEST_F(Test3, Ported) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value run_callback = nullptr;
  napi_value run_callback_with_recv = nullptr;
  ASSERT_EQ(napi_get_named_property(s.env, exports, "RunCallback", &run_callback), napi_ok);
  ASSERT_EQ(napi_get_named_property(s.env, exports, "RunCallbackWithRecv", &run_callback_with_recv), napi_ok);

  bool saw_hello = false;
  napi_value cb = nullptr;
  ASSERT_EQ(napi_create_function(s.env, "verifyHello", NAPI_AUTO_LENGTH, VerifyHello, &saw_hello, &cb), napi_ok);
  napi_value argv[1] = {cb};
  ASSERT_EQ(napi_call_function(s.env, exports, run_callback, 1, argv, nullptr), napi_ok);
  EXPECT_TRUE(saw_hello);

  v8::Local<v8::String> source = v8::String::NewFromUtf8Literal(
      s.isolate, "(function(){ 'use strict'; globalThis.__recvSeen = this; })");
  v8::Local<v8::Script> script = v8::Script::Compile(s.context, source).ToLocalChecked();
  v8::Local<v8::Value> callback_value = script->Run(s.context).ToLocalChecked();
  napi_value recv_cb = nullptr;
  ASSERT_EQ(napi_v8_wrap_existing_value(s.env, callback_value, &recv_cb), napi_ok);

  std::vector<v8::Local<v8::Value>> recv_values = {
      v8::Undefined(s.isolate),
      v8::Null(s.isolate),
      v8::Number::New(s.isolate, 5),
      v8::Boolean::New(s.isolate, true),
      v8::String::NewFromUtf8Literal(s.isolate, "Hello"),
      v8::Array::New(s.isolate),
      v8::Object::New(s.isolate)};

  for (auto recv_v8 : recv_values) {
    napi_value recv = nullptr;
    ASSERT_EQ(napi_v8_wrap_existing_value(s.env, recv_v8, &recv), napi_ok);
    napi_value args[2] = {recv_cb, recv};
    ASSERT_EQ(napi_call_function(s.env, exports, run_callback_with_recv, 2, args, nullptr), napi_ok);
    v8::Local<v8::Value> seen =
        s.context->Global()
            ->Get(s.context, v8::String::NewFromUtf8Literal(s.isolate, "__recvSeen"))
            .ToLocalChecked();
    EXPECT_TRUE(seen->StrictEquals(recv_v8));
  }
}
