#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

#include "test_env.h"
#include "unode_cli.h"

class Test1CliPhase01 : public FixtureTestBase {};

namespace {

std::string WriteTempScript(const std::string& stem, const std::string& contents) {
  const auto temp_dir = std::filesystem::temp_directory_path();
  const auto unique_name =
      stem + "_" + std::to_string(static_cast<unsigned long long>(std::hash<std::string>{}(contents))) + ".js";
  const auto script_path = temp_dir / unique_name;
  std::ofstream out(script_path);
  out << contents;
  out.close();
  return script_path.string();
}

void RemoveTempScript(const std::string& path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

}  // namespace

TEST_F(Test1CliPhase01, MissingScriptArgReturnsUsageError) {
  const char* argv[] = {"unode"};
  std::string error;
  const int exit_code = UnodeRunCli(1, argv, &error);
  EXPECT_EQ(exit_code, 1);
  EXPECT_EQ(error, "Usage: unode <script.js>");
}

TEST_F(Test1CliPhase01, ExtraArgsReturnUsageError) {
  const char* argv[] = {"unode", "first.js", "second.js"};
  std::string error;
  const int exit_code = UnodeRunCli(3, argv, &error);
  EXPECT_EQ(exit_code, 1);
  EXPECT_EQ(error, "Usage: unode <script.js>");
}

TEST_F(Test1CliPhase01, MissingScriptFileReturnsNonZero) {
  const std::string script_path =
      std::string(NAPI_V8_ROOT_PATH) + "/tests/fixtures/this_file_should_not_exist_phase01.js";
  const char* argv[] = {"unode", script_path.c_str()};
  std::string error;
  const int exit_code = UnodeRunCli(2, argv, &error);
  EXPECT_EQ(exit_code, 1);
  EXPECT_EQ(error, "Failed to read script file");
}

TEST_F(Test1CliPhase01, ValidFixtureScriptReturnsZero) {
  const std::string script_path = std::string(NAPI_V8_ROOT_PATH) + "/tests/fixtures/phase0_hello.js";
  ASSERT_TRUE(std::filesystem::exists(script_path)) << "Missing fixture: " << script_path;
  const char* argv[] = {"unode", script_path.c_str()};

  testing::internal::CaptureStdout();
  std::string error;
  const int exit_code = UnodeRunCli(2, argv, &error);
  const std::string stdout_output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(exit_code, 0) << "error=" << error;
  EXPECT_TRUE(error.empty()) << "error=" << error;
  EXPECT_NE(stdout_output.find("hello from unode"), std::string::npos);
}

TEST_F(Test1CliPhase01, RuntimeThrownErrorReturnsNonZero) {
  const std::string script_path = WriteTempScript("unode_phase01_cli_throw", "throw new Error('boom from cli');");
  const char* argv[] = {"unode", script_path.c_str()};
  std::string error;

  const int exit_code = UnodeRunCli(2, argv, &error);
  EXPECT_EQ(exit_code, 1);
  EXPECT_NE(error.find("boom from cli"), std::string::npos);

  RemoveTempScript(script_path);
}

TEST_F(Test1CliPhase01, RuntimeSyntaxErrorReturnsNonZero) {
  const std::string script_path = WriteTempScript("unode_phase01_cli_syntax", "function (");
  const char* argv[] = {"unode", script_path.c_str()};
  std::string error;

  const int exit_code = UnodeRunCli(2, argv, &error);
  EXPECT_EQ(exit_code, 1);
  EXPECT_FALSE(error.empty());

  RemoveTempScript(script_path);
}
