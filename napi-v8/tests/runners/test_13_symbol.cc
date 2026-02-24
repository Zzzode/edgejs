#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test13Symbol : public FixtureTestBase {};

TEST_F(Test13Symbol, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value global = nullptr;
  ASSERT_EQ(napi_get_global(s.env, &global), napi_ok);
  ASSERT_EQ(napi_set_named_property(s.env, global, "__sym", exports), napi_ok);

  auto run_js = [&](const char* source_text) {
    v8::TryCatch tc(s.isolate);
    std::string wrapped = std::string("(() => { ") + source_text + " })();";
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

  ASSERT_TRUE(run_js("const s=__sym.New('test'); if(s.toString()!=='Symbol(test)') throw new Error('s1');"));
  ASSERT_TRUE(run_js("const myObj={}; const fooSym=__sym.New('foo'); const otherSym=__sym.New('bar'); myObj.foo='bar'; myObj[fooSym]='baz'; myObj[otherSym]='bing'; if(myObj.foo!=='bar'||myObj[fooSym]!=='baz'||myObj[otherSym]!=='bing') throw new Error('s2');"));
  ASSERT_TRUE(run_js("const fooSym=__sym.New('foo'); const myObj={}; myObj.foo='bar'; myObj[fooSym]='baz'; const keys=Object.keys(myObj); const names=Object.getOwnPropertyNames(myObj); const syms=Object.getOwnPropertySymbols(myObj); if(keys.length!==1||keys[0]!=='foo'||names.length!==1||names[0]!=='foo'||syms.length!==1||syms[0]!==fooSym) throw new Error('s3');"));
  ASSERT_TRUE(run_js("if(__sym.New()===__sym.New()) throw new Error('s4'); if(__sym.New('foo')===__sym.New('foo')) throw new Error('s5'); if(__sym.New('foo')===__sym.New('bar')) throw new Error('s6'); const foo1=__sym.New('foo'); const foo2=__sym.New('foo'); const object={[foo1]:1,[foo2]:2}; if(object[foo1]!==1||object[foo2]!==2) throw new Error('s7');"));
}
