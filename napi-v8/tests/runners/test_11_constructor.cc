#include <string>

#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test11Constructor : public FixtureTestBase {};

TEST_F(Test11Constructor, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  napi_value ctor = Init(s.env, exports);
  ASSERT_NE(ctor, nullptr);

  napi_value global = nullptr;
  ASSERT_EQ(napi_get_global(s.env, &global), napi_ok);
  ASSERT_EQ(napi_set_named_property(s.env, global, "__Ctor", ctor), napi_ok);

  auto run_js = [&](const char* source_text) {
    v8::TryCatch tc(s.isolate);
    std::string wrapped = std::string("\"use strict\";\n") + source_text;
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
    return true;
  };

  ASSERT_TRUE(run_js("globalThis.__o = new __Ctor();"));
  ASSERT_TRUE(run_js("if(__o.echo('hello')!=='hello') throw new Error('echo');"));
  ASSERT_TRUE(run_js("__o.readwriteValue=1; if(__o.readwriteValue!==1) throw new Error('rw1');"));
  ASSERT_TRUE(run_js("__o.readwriteValue=2; if(__o.readwriteValue!==2) throw new Error('rw2');"));
  ASSERT_TRUE(run_js("let threw=false; try { __o.readonlyValue=3; } catch(_) { threw=true; } if(!threw) throw new Error('ro');"));
  ASSERT_TRUE(run_js("if(!__o.hiddenValue) throw new Error('hidden');"));
  ASSERT_TRUE(run_js("const n=[]; for (const k in __o) n.push(k); if(!n.includes('echo')||!n.includes('readwriteValue')||!n.includes('readonlyValue')) throw new Error('enum1'); if(n.includes('hiddenValue')||n.includes('readwriteAccessor1')||n.includes('readwriteAccessor2')||n.includes('readonlyAccessor1')||n.includes('readonlyAccessor2')) throw new Error('enum2');"));
  ASSERT_TRUE(run_js("__o.readwriteAccessor1=1; if(__o.readwriteAccessor1!==1||__o.readonlyAccessor1!==1) throw new Error('acc1');"));
  ASSERT_TRUE(run_js("let t1=false; try { __o.readonlyAccessor1=3; } catch(_) { t1=true; } if(!t1) throw new Error('acc1ro');"));
  ASSERT_TRUE(run_js("__o.readwriteAccessor2=2; if(__o.readwriteAccessor2!==2||__o.readonlyAccessor2!==2) throw new Error('acc2');"));
  ASSERT_TRUE(run_js("let t2=false; try { __o.readonlyAccessor2=3; } catch(_) { t2=true; } if(!t2) throw new Error('acc2ro');"));
  ASSERT_TRUE(run_js("if(__Ctor.staticReadonlyAccessor1!==10) throw new Error('static1');"));
  ASSERT_TRUE(run_js("if(__o.staticReadonlyAccessor1!==undefined) throw new Error('static2');"));
  ASSERT_TRUE(run_js("const r=__Ctor.TestDefineClass(); if(!r||r.envIsNull!=='Invalid argument'||r.nameIsNull!=='Invalid argument'||r.cbIsNull!=='Invalid argument'||r.cbDataIsNull!=='napi_ok'||r.propertiesIsNull!=='Invalid argument'||r.resultIsNull!=='Invalid argument') throw new Error('defineClass');"));
}
