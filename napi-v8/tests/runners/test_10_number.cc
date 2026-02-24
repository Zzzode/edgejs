#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test10Number : public FixtureTestBase {};

TEST_F(Test10Number, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value global = nullptr;
  ASSERT_EQ(napi_get_global(s.env, &global), napi_ok);
  ASSERT_EQ(napi_set_named_property(s.env, global, "__addon", exports), napi_ok);

  auto run_js = [&](const char* source_text) {
    v8::TryCatch tc(s.isolate);
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(s.isolate, source_text, v8::NewStringType::kNormal)
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
    return true;
  };

  ASSERT_TRUE(run_js("if(__addon.Test(0)!==0) throw new Error('t0');"));
  ASSERT_TRUE(run_js("if(__addon.Test(-1)!==-1) throw new Error('t1');"));
  ASSERT_TRUE(run_js("if(__addon.Test(2121)!==2121) throw new Error('t2');"));
  ASSERT_TRUE(run_js("if(__addon.Test(Number.MAX_SAFE_INTEGER)!==Number.MAX_SAFE_INTEGER) throw new Error('t3');"));
  ASSERT_TRUE(run_js("if(!Number.isNaN(__addon.Test(Number.NaN))) throw new Error('tnan');"));

  ASSERT_TRUE(run_js("if(__addon.TestUint32Truncation(4294967297)!==1) throw new Error('u1');"));
  ASSERT_TRUE(run_js("if(__addon.TestUint32Truncation(-1)!==0xffffffff) throw new Error('u2');"));

  ASSERT_TRUE(run_js("if(__addon.TestInt32Truncation(4294967295)!==-1) throw new Error('i1');"));
  ASSERT_TRUE(run_js("if(__addon.TestInt32Truncation(4294967296)!==0) throw new Error('i2');"));

  ASSERT_TRUE(run_js("if(__addon.TestInt64Truncation(0)!==0) throw new Error('i640');"));
  ASSERT_TRUE(run_js("if(__addon.TestInt64Truncation(Number.MAX_SAFE_INTEGER)!==Number.MAX_SAFE_INTEGER) throw new Error('i641');"));
}
