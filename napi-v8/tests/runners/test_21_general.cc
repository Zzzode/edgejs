#include <string>

#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test21General : public FixtureTestBase {};

TEST_F(Test21General, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value global = nullptr;
  ASSERT_EQ(napi_get_global(s.env, &global), napi_ok);
  ASSERT_EQ(napi_set_named_property(s.env, global, "__gen", exports), napi_ok);

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

  ASSERT_TRUE(run_js(R"JS(
const val1 = '1';
const val2 = 1;
const val3 = 1;
if (!__gen.testStrictEquals(val1, val1)) throw new Error('strict1');
if (__gen.testStrictEquals(val1, val2) !== false) throw new Error('strict2');
if (!__gen.testStrictEquals(val2, val3)) throw new Error('strict3');
const nullProtoObject = { __proto__: null };
__gen.testSetPrototype(nullProtoObject, Object.prototype);
if (Object.getPrototypeOf(nullProtoObject) !== Object.prototype) throw new Error('setProto');
class BaseClass {}
class ExtendedClass extends BaseClass {}
const baseObject = new BaseClass();
const extendedObject = new ExtendedClass();
if (__gen.testGetPrototype(baseObject) !== Object.getPrototypeOf(baseObject)) throw new Error('getProto1');
if (__gen.testGetPrototype(extendedObject) !== Object.getPrototypeOf(extendedObject)) throw new Error('getProto2');
if (__gen.testGetPrototype(baseObject) === __gen.testGetPrototype(extendedObject)) throw new Error('getProto3');
if (__gen.testGetVersion() !== 10) throw new Error('version');
[123, 'test string', function(){}, {}, true, undefined, Symbol()].forEach((v) => {
  if (__gen.testNapiTypeof(v) !== typeof v) throw new Error('typeof');
});
if (__gen.testNapiTypeof(null) !== 'null') throw new Error('typeofNull');
const x = {};
__gen.wrap(x);
let threw = false;
try { __gen.wrap(x); } catch (e) { threw = (e && e.message === 'Invalid argument'); }
if (!threw) throw new Error('wrapTwice');
__gen.removeWrap(x);
const y = {};
__gen.wrap(y);
__gen.removeWrap(y);
__gen.wrap(y);
__gen.removeWrap(y);
const adjustedValue = __gen.testAdjustExternalMemory();
if (typeof adjustedValue !== 'number' || !(adjustedValue > 0)) throw new Error('adjust');
)JS"));
}
