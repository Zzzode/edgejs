#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test6 : public FixtureTestBase {};

TEST_F(Test6, PortedCoreFlow) {
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

  ASSERT_TRUE(run_js("\"use strict\"; const d1=Object.getOwnPropertyDescriptor(__addon.MyObject.prototype,'value'); if(typeof d1.get!=='function'||typeof d1.set!=='function'||d1.value!==undefined||d1.enumerable!==false||d1.configurable!==false) throw new Error('value descriptor');"));
  ASSERT_TRUE(run_js("\"use strict\"; const d2=Object.getOwnPropertyDescriptor(__addon.MyObject.prototype,'valueReadonly'); if(typeof d2.get!=='function'||d2.set!==undefined||d2.value!==undefined||d2.enumerable!==false||d2.configurable!==false) throw new Error('readonly descriptor');"));
  ASSERT_TRUE(run_js("\"use strict\"; const d3=Object.getOwnPropertyDescriptor(__addon.MyObject.prototype,'plusOne'); if(d3.get!==undefined||d3.set!==undefined||typeof d3.value!=='function'||d3.enumerable!==false||d3.configurable!==false) throw new Error('plusOne descriptor');"));

  ASSERT_TRUE(run_js("\"use strict\"; globalThis.__obj = new __addon.MyObject(9);"));
  ASSERT_TRUE(run_js("\"use strict\"; if (__obj.value !== 9) throw new Error('value');"));
  ASSERT_TRUE(run_js("\"use strict\"; __obj.value = 10;"));
  ASSERT_TRUE(run_js("\"use strict\"; if (__obj.value !== 10) throw new Error('value2');"));
  ASSERT_TRUE(run_js("\"use strict\"; if (__obj.valueReadonly !== 10) throw new Error('readonly value');"));
  ASSERT_TRUE(run_js("\"use strict\"; let threw=false; try { __obj.valueReadonly=14; } catch(_) { threw=true; } if(!threw) throw new Error('readonly assign');"));
  ASSERT_TRUE(run_js("\"use strict\"; if (__obj.plusOne() !== 11) throw new Error('plus1');"));
  ASSERT_TRUE(run_js("\"use strict\"; if (__obj.plusOne() !== 12) throw new Error('plus2');"));
  ASSERT_TRUE(run_js("\"use strict\"; if (__obj.plusOne() !== 13) throw new Error('plus3');"));
  ASSERT_TRUE(run_js("\"use strict\"; if (__obj.multiply().value !== 13) throw new Error('multiply1');"));
  ASSERT_TRUE(run_js("\"use strict\"; if (__obj.multiply(10).value !== 130) throw new Error('multiply2');"));
  ASSERT_TRUE(run_js("\"use strict\"; const newobj=__obj.multiply(-1); if(newobj.value!==-13||newobj.valueReadonly!==-13||newobj===__obj) throw new Error('multiply3');"));
}
