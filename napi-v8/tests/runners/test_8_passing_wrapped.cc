#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test8 : public FixtureTestBase {};

TEST_F(Test8, PortedCoreFlow) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  ASSERT_NE(Init(s.env, exports), nullptr);

  napi_value create_object = nullptr;
  napi_value add = nullptr;
  ASSERT_EQ(napi_get_named_property(s.env, exports, "createObject", &create_object), napi_ok);
  ASSERT_EQ(napi_get_named_property(s.env, exports, "add", &add), napi_ok);

  napi_value ten = nullptr;
  napi_value twenty = nullptr;
  ASSERT_EQ(napi_create_double(s.env, 10, &ten), napi_ok);
  ASSERT_EQ(napi_create_double(s.env, 20, &twenty), napi_ok);

  napi_value obj1 = nullptr;
  napi_value obj2 = nullptr;
  napi_value argv1[1] = {ten};
  napi_value argv2[1] = {twenty};
  ASSERT_EQ(napi_call_function(s.env, exports, create_object, 1, argv1, &obj1), napi_ok);
  ASSERT_EQ(napi_call_function(s.env, exports, create_object, 1, argv2, &obj2), napi_ok);

  napi_value add_args[2] = {obj1, obj2};
  napi_value sum = nullptr;
  ASSERT_EQ(napi_call_function(s.env, exports, add, 2, add_args, &sum), napi_ok);
  double value = 0;
  ASSERT_EQ(napi_get_value_double(s.env, sum, &value), napi_ok);
  EXPECT_DOUBLE_EQ(value, 30);
}
