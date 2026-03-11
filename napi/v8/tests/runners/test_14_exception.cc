#include "test_env.h"
#include "upstream_js_test.h"
#include "unofficial_napi.h"

extern "C" napi_value Init(napi_env env, napi_value exports);

class Test14Exception : public FixtureTestBase {};

namespace {

std::string ValueToUtf8(napi_env env, napi_value value) {
  if (env == nullptr || value == nullptr) return {};
  size_t length = 0;
  if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) return {};
  std::string out(length + 1, '\0');
  size_t copied = 0;
  if (napi_get_value_string_utf8(env, value, out.data(), out.size(), &copied) != napi_ok) return {};
  out.resize(copied);
  return out;
}

}  // namespace

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
  ASSERT_TRUE(InstallUpstreamJsShim(s, binding));
  ASSERT_TRUE(SetUpstreamRequireException(s, init_error));
  ASSERT_TRUE(RunUpstreamJsFile(
      s, std::string(NAPI_TESTS_ROOT_PATH) + "/js-native-api/test_exception/test.js"));
}

TEST_F(Test14Exception, GetAndClearPendingExceptionNoPending) {
  EnvScope s(runtime_.get());

  unofficial_napi_pending_exception_info info = {};
  ASSERT_EQ(unofficial_napi_get_and_clear_pending_exception(s.env, &info), napi_ok);
  EXPECT_FALSE(info.has_exception);
  EXPECT_EQ(info.exception, nullptr);
  EXPECT_EQ(info.exception_line, nullptr);
}

TEST_F(Test14Exception, GetAndClearPendingExceptionCapturesLineAndDecoratesStack) {
  EnvScope s(runtime_.get());

  napi_value script = nullptr;
  ASSERT_EQ(
      napi_create_string_utf8(
          s.env, "throw new Error('boom')", NAPI_AUTO_LENGTH, &script),
      napi_ok);
  napi_value result = nullptr;
  ASSERT_EQ(napi_run_script(s.env, script, &result), napi_pending_exception);

  unofficial_napi_pending_exception_info info = {};
  ASSERT_EQ(unofficial_napi_get_and_clear_pending_exception(s.env, &info), napi_ok);
  ASSERT_TRUE(info.has_exception);
  ASSERT_NE(info.exception, nullptr);
  ASSERT_NE(info.exception_line, nullptr);
  EXPECT_NE(ValueToUtf8(s.env, info.exception_line).find("throw new Error('boom')"), std::string::npos);

  napi_value stack = nullptr;
  ASSERT_EQ(napi_get_named_property(s.env, info.exception, "stack", &stack), napi_ok);
  EXPECT_NE(ValueToUtf8(s.env, stack).find("throw new Error('boom')"), std::string::npos);
}

TEST_F(Test14Exception, GetAndClearPendingExceptionPreservesMessageAcrossRethrow) {
  EnvScope s(runtime_.get());

  napi_value script = nullptr;
  ASSERT_EQ(
      napi_create_string_utf8(
          s.env, "throw new Error('boom')", NAPI_AUTO_LENGTH, &script),
      napi_ok);
  napi_value result = nullptr;
  ASSERT_EQ(napi_run_script(s.env, script, &result), napi_pending_exception);

  unofficial_napi_pending_exception_info first = {};
  ASSERT_EQ(unofficial_napi_get_and_clear_pending_exception(s.env, &first), napi_ok);
  ASSERT_TRUE(first.has_exception);
  ASSERT_NE(first.exception, nullptr);
  ASSERT_NE(first.exception_line, nullptr);
  const std::string first_line = ValueToUtf8(s.env, first.exception_line);
  ASSERT_FALSE(first_line.empty());

  ASSERT_EQ(napi_throw(s.env, first.exception), napi_pending_exception);

  unofficial_napi_pending_exception_info second = {};
  ASSERT_EQ(unofficial_napi_get_and_clear_pending_exception(s.env, &second), napi_ok);
  ASSERT_TRUE(second.has_exception);
  ASSERT_NE(second.exception, nullptr);
  ASSERT_NE(second.exception_line, nullptr);
  EXPECT_EQ(ValueToUtf8(s.env, second.exception_line), first_line);
}
