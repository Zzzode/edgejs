#include <string>

#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test19Properties : public FixtureTestBase {};

TEST_F(Test19Properties, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value global = nullptr;
  ASSERT_EQ(napi_get_global(s.env, &global), napi_ok);
  ASSERT_EQ(napi_set_named_property(s.env, global, "__prop", exports), napi_ok);

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
'use strict';
const readonlyErrorRE = /^TypeError: Cannot assign to read only property '.*' of object '#<Object>'$/;
const getterOnlyErrorRE = /^TypeError: Cannot set property .* of #<Object> which has only a getter$/;
if (__prop.echo('hello') !== 'hello') throw new Error('echo');
__prop.readwriteValue = 1;
if (__prop.readwriteValue !== 1) throw new Error('rw1');
__prop.readwriteValue = 2;
if (__prop.readwriteValue !== 2) throw new Error('rw2');
let threw = false;
try { __prop.readonlyValue = 3; } catch (e) { threw = readonlyErrorRE.test(String(e)); }
if (!threw) throw new Error('readonly');
if (!__prop.hiddenValue) throw new Error('hidden');
const propertyNames = [];
for (const name in __prop) propertyNames.push(name);
if (!propertyNames.includes('echo')) throw new Error('enumEcho');
if (!propertyNames.includes('readwriteValue')) throw new Error('enumRw');
if (!propertyNames.includes('readonlyValue')) throw new Error('enumRo');
if (propertyNames.includes('hiddenValue')) throw new Error('enumHidden');
if (!propertyNames.includes('NameKeyValue')) throw new Error('enumNameVal');
if (propertyNames.includes('readwriteAccessor1')) throw new Error('enumAcc1');
if (propertyNames.includes('readwriteAccessor2')) throw new Error('enumAcc2');
if (propertyNames.includes('readonlyAccessor1')) throw new Error('enumAcc3');
if (propertyNames.includes('readonlyAccessor2')) throw new Error('enumAcc4');
const propertySymbols = Object.getOwnPropertySymbols(__prop);
if (propertySymbols[0].toString() !== 'Symbol(NameKeySymbol)') throw new Error('sym0');
if (propertySymbols[1].toString() !== 'Symbol()') throw new Error('sym1');
if (propertySymbols[2] !== Symbol.for('NameKeySymbolFor')) throw new Error('sym2');
const readwriteAccessor1Descriptor = Object.getOwnPropertyDescriptor(__prop, 'readwriteAccessor1');
const readonlyAccessor1Descriptor = Object.getOwnPropertyDescriptor(__prop, 'readonlyAccessor1');
if (readwriteAccessor1Descriptor.get == null) throw new Error('accGet1');
if (readwriteAccessor1Descriptor.set == null) throw new Error('accSet1');
if (readwriteAccessor1Descriptor.value !== undefined) throw new Error('accVal1');
if (readonlyAccessor1Descriptor.get == null) throw new Error('accGet2');
if (readonlyAccessor1Descriptor.set !== undefined) throw new Error('accSet2');
if (readonlyAccessor1Descriptor.value !== undefined) throw new Error('accVal2');
__prop.readwriteAccessor1 = 1;
if (__prop.readwriteAccessor1 !== 1) throw new Error('accRw1');
if (__prop.readonlyAccessor1 !== 1) throw new Error('accRo1');
threw = false;
try { __prop.readonlyAccessor1 = 3; } catch (e) { threw = getterOnlyErrorRE.test(String(e)); }
if (!threw) throw new Error('accThrow1');
__prop.readwriteAccessor2 = 2;
if (__prop.readwriteAccessor2 !== 2) throw new Error('accRw2');
if (__prop.readonlyAccessor2 !== 2) throw new Error('accRo2');
threw = false;
try { __prop.readonlyAccessor2 = 3; } catch (e) { threw = getterOnlyErrorRE.test(String(e)); }
if (!threw) throw new Error('accThrow2');
if (__prop.hasNamedProperty(__prop, 'echo') !== true) throw new Error('hasNamed1');
if (__prop.hasNamedProperty(__prop, 'hiddenValue') !== true) throw new Error('hasNamed2');
if (__prop.hasNamedProperty(__prop, 'doesnotexist') !== false) throw new Error('hasNamed3');
)JS"));
}
