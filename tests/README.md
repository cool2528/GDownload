# GDownload 测试框架

本文档描述了如何使用 GDownload 项目的单元测试和集成测试框架。

## 目录结构

```
tests/
├── CMakeLists.txt          # 测试框架主配置
├── README.md               # 本文档
├── unit/                   # 单元测试
│   ├── CMakeLists.txt      # 单元测试配置
│   ├── main.cpp            # 单元测试主程序
│   ├── utils/              # 工具类测试
│   │   ├── test_utils_manager.h
│   │   └── test_utils_manager.cpp
│   └── core/               # 核心模块测试
│       └── test_core_functions.cpp
└── integration/            # 集成测试
    ├── CMakeLists.txt      # 集成测试配置
    ├── main.cpp            # 集成测试主程序
    └── test_relaunch_after_exit.cpp
```

## 测试框架

- **Google Test (gtest)**: 单元测试框架
- **Google Mock (gmock)**: Mock 对象框架
- **CTest**: CMake 集成测试工具

## 构建和运行测试

### 1. 启用测试构建

```bash
# 配置时启用测试
cmake -B build -S . -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# 构建项目（包括测试）
cmake --build build --config Debug
```

### 2. 运行所有测试

```bash
# 使用 CMake 运行测试
cd build
ctest --output-on-failure

# 或者直接运行测试可执行文件
./Debug/bin/Debug/gdownload_unit_tests
./Debug/bin/Debug/gdownload_integration_tests
```

### 3. 运行特定测试

```bash
# 运行单元测试
./Debug/bin/Debug/gdownload_unit_tests --gtest_filter="UtilsManagerTest.*"

# 运行集成测试
./Debug/bin/Debug/gdownload_integration_tests --gtest_filter="RelaunchAfterExitTest.*"

# 列出所有测试（不运行）
./Debug/bin/Debug/gdownload_unit_tests --gtest_list_tests
```

### 4. 测试输出选项

```bash
# 详细输出
./Debug/bin/Debug/gdownload_unit_tests --gtest_print_time=1

# 输出到 XML 文件
./Debug/bin/Debug/gdownload_unit_tests --gtest_output=xml:test_results.xml

# 只运行失败的测试
./Debug/bin/Debug/gdownload_unit_tests --gtest_filter="*FAILED"
```

## 测试内容

### 单元测试 (Unit Tests)

1. **UtilsManagerTest**: 测试工具管理器的各种功能
   - 版本信息获取
   - 剪贴板操作
   - 自启动功能
   - RelaunchAfterExit 基础功能

2. **CoreFunctionsTest**: 测试核心模块功能
   - 日志系统
   - 配置管理
   - 版本信息

### 集成测试 (Integration Tests)

1. **RelaunchAfterExitTest**: 测试实际的重启行为
   - 验证重启后的进程行为
   - 不同延迟时间的重启测试
   - 平台特定的重启实现
   - 错误恢复和性能测试

## 自定义测试目标

项目提供了几个便捷的测试目标：

```bash
# 运行所有测试（详细输出）
cmake --build build --target run_tests

# 清理测试结果
cmake --build build --target clean_tests

# 生成覆盖率报告（需要 gcov 和 lcov）
cmake --build build --target coverage
```

## 编写新测试

### 添加新的单元测试

1. 在 `tests/unit/` 下创建新的测试文件
2. 继承 `::testing::Test` 类
3. 使用 `TEST_F()` 宏编写测试用例
4. 在对应的 `CMakeLists.txt` 中添加新的测试文件

### 添加新的集成测试

1. 在 `tests/integration/` 下创建新的测试文件
2. 遵循类似的测试结构
3. 注意集成测试可能需要更长的时间来完成

### 示例测试结构

```cpp
#include <gtest/gtest.h>

class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前准备
    }

    void TearDown() override {
        // 测试后清理
    }
};

TEST_F(MyTest, TestBasicFunctionality) {
    EXPECT_TRUE(true) << "Basic test";
}
```

## 调试测试

### 调试失败的测试

1. 使用详细的输出选项运行测试
2. 在代码中添加调试输出 (`qDebug()`, `std::cout`)
3. 使用断点调试器附加到测试进程

### 常见问题

1. **测试找不到依赖库**: 确保库文件在正确的路径
2. **测试超时**: 增加超时时间或检查测试逻辑
3. **资源清理失败**: 确保 `TearDown()` 方法正确清理资源

## 持续集成

这些测试可以集成到 CI/CD 流水线中：

```yaml
# GitHub Actions 示例
- name: Run Tests
  run: |
    cmake -B build -S . -DBUILD_TESTS=ON
    cmake --build build
    cd build
    ctest --output-on-failure --timeout 300
```

## 覆盖率报告

在 Debug 模式下，如果安装了 `gcov` 和 `lcov`，可以生成覆盖率报告：

```bash
cmake --build build --target coverage
# 报告将生成在 build/coverage_html/ 目录
```

## 注意事项

1. 测试应该在独立的环境中运行
2. 避免测试之间相互依赖
3. 集成测试可能需要特殊的环境配置
4. 及时清理测试产生的临时文件和进程
5. 定期更新和维护测试用例