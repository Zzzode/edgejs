#include <string>

#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test14Exception : public FixtureTestBase {};

TEST_F(Test14Exception, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  (void)Init(s.env, exports);

  bool pending = false;
  ASSERT_EQ(napi_is_exception_pending(s.env, &pending), napi_ok);
  ASSERT_TRUE(pending);
  napi_value init_error = nullptr;
  ASSERT_EQ(napi_get_and_clear_last_exception(s.env, &init_error), napi_ok);

  napi_value binding = nullptr;
  ASSERT_EQ(napi_get_named_property(s.env, init_error, "binding", &binding), napi_ok);

  napi_value global = nullptr;
  ASSERT_EQ(napi_get_global(s.env, &global), napi_ok);
  ASSERT_EQ(napi_set_named_property(s.env, global, "__exc", binding), napi_ok);

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

  ASSERT_TRUE(run_js("const theError=new Error('Some error'); const throwTheError=()=>{throw theError;}; const returned=__exc.returnException(throwTheError); if(returned!==theError) throw new Error('returnException');"));
  ASSERT_TRUE(run_js("const theError=new Error('Some error'); const throwTheError=()=>{throw theError;}; let ok=false; try { __exc.allowException(throwTheError); } catch(e) { ok=(e===theError); } if(!ok) throw new Error('allowException');"));
  ASSERT_TRUE(run_js("if(__exc.wasPending()!==true) throw new Error('wasPendingTrue');"));
  ASSERT_TRUE(run_js("const returned=__exc.returnException(() => {}); if(returned!==undefined) throw new Error('returnUndefined');"));
  ASSERT_TRUE(run_js("const theError=new Error('Some error'); const ThrowCls=class { constructor() { throw theError; } }; const returned=__exc.constructReturnException(ThrowCls); if(returned!==theError) throw new Error('constructReturnException');"));
  ASSERT_TRUE(run_js("const theError=new Error('Some error'); const ThrowCls=class { constructor() { throw theError; } }; let ok=false; try { __exc.constructAllowException(ThrowCls); } catch(e) { ok=(e===theError); } if(!ok) throw new Error('constructAllowException');"));
  ASSERT_TRUE(run_js("if(__exc.wasPending()!==true) throw new Error('wasPendingTrue2');"));
  ASSERT_TRUE(run_js("let caught; try { __exc.allowException(() => {}); } catch(e) { caught=e; } if(caught!==undefined) throw new Error('noCatch1');"));
  ASSERT_TRUE(run_js("if(__exc.wasPending()!==false) throw new Error('wasPendingFalse1');"));
  ASSERT_TRUE(run_js("let caught; try { __exc.constructAllowException(class { constructor() {} }); } catch(e) { caught=e; } if(caught!==undefined) throw new Error('noCatch2');"));
  ASSERT_TRUE(run_js("if(__exc.wasPending()!==false) throw new Error('wasPendingFalse2');"));
}
