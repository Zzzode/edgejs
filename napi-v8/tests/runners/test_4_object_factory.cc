#include <string>

#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test4 : public FixtureTestBase {};

TEST_F(Test4, Ported) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  napi_value addon = Init(s.env, exports);
  ASSERT_NE(addon, nullptr);

  napi_value hello = nullptr;
  napi_value world = nullptr;
  ASSERT_EQ(napi_create_string_utf8(s.env, "hello", NAPI_AUTO_LENGTH, &hello), napi_ok);
  ASSERT_EQ(napi_create_string_utf8(s.env, "world", NAPI_AUTO_LENGTH, &world), napi_ok);

  napi_value argv1[1] = {hello};
  napi_value argv2[1] = {world};
  napi_value obj1 = nullptr;
  napi_value obj2 = nullptr;
  ASSERT_EQ(napi_call_function(s.env, exports, addon, 1, argv1, &obj1), napi_ok);
  ASSERT_EQ(napi_call_function(s.env, exports, addon, 1, argv2, &obj2), napi_ok);

  napi_value msg1 = nullptr;
  napi_value msg2 = nullptr;
  ASSERT_EQ(napi_get_named_property(s.env, obj1, "msg", &msg1), napi_ok);
  ASSERT_EQ(napi_get_named_property(s.env, obj2, "msg", &msg2), napi_ok);
  char b1[16];
  char b2[16];
  ASSERT_EQ(napi_get_value_string_utf8(s.env, msg1, b1, sizeof(b1), nullptr), napi_ok);
  ASSERT_EQ(napi_get_value_string_utf8(s.env, msg2, b2, sizeof(b2), nullptr), napi_ok);
  EXPECT_EQ(std::string(b1), "hello");
  EXPECT_EQ(std::string(b2), "world");
}
