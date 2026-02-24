#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test2 : public FixtureTestBase {};

TEST_F(Test2, Ported) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value add_fn = nullptr;
  ASSERT_EQ(napi_get_named_property(s.env, exports, "add", &add_fn), napi_ok);

  napi_value a = nullptr;
  napi_value b = nullptr;
  ASSERT_EQ(napi_create_double(s.env, 3, &a), napi_ok);
  ASSERT_EQ(napi_create_double(s.env, 5, &b), napi_ok);
  napi_value argv[2] = {a, b};
  napi_value out = nullptr;
  ASSERT_EQ(napi_call_function(s.env, exports, add_fn, 2, argv, &out), napi_ok);
  double value = 0;
  ASSERT_EQ(napi_get_value_double(s.env, out, &value), napi_ok);
  EXPECT_DOUBLE_EQ(value, 8);
}
