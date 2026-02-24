#include <string>

#include "test_env.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test5 : public FixtureTestBase {};

TEST_F(Test5, Ported) {
  EnvScope s(runtime_.get());
  napi_value exports = nullptr;
  ASSERT_EQ(napi_create_object(s.env, &exports), napi_ok);
  napi_value addon = Init(s.env, exports);
  ASSERT_NE(addon, nullptr);

  napi_value fn = nullptr;
  ASSERT_EQ(napi_call_function(s.env, exports, addon, 0, nullptr, &fn), napi_ok);
  napi_value out = nullptr;
  ASSERT_EQ(napi_call_function(s.env, exports, fn, 0, nullptr, &out), napi_ok);
  char buf[32];
  ASSERT_EQ(napi_get_value_string_utf8(s.env, out, buf, sizeof(buf), nullptr), napi_ok);
  EXPECT_EQ(std::string(buf), "hello world");
}
