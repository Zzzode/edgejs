#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test7 : public FixtureTestBase {};

TEST_F(Test7, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value create_object = nullptr;
  ASSERT_EQ(napi_get_named_property(s.env, exports, "createObject", &create_object), napi_ok);
  napi_value arg = nullptr;
  ASSERT_EQ(napi_create_uint32(s.env, 10, &arg), napi_ok);

  napi_value argv[1] = {arg};
  napi_value obj = nullptr;
  ASSERT_EQ(napi_call_function(s.env, exports, create_object, 1, argv, &obj), napi_ok);

  napi_value plus_one = nullptr;
  ASSERT_EQ(napi_get_named_property(s.env, obj, "plusOne", &plus_one), napi_ok);

  napi_value out = nullptr;
  ASSERT_EQ(napi_call_function(s.env, obj, plus_one, 0, nullptr, &out), napi_ok);
  uint32_t v = 0;
  ASSERT_EQ(napi_get_value_uint32(s.env, out, &v), napi_ok);
  EXPECT_EQ(v, 11u);
}
