cmake_minimum_required(VERSION 3.16)
project(dps_simulator)

# 把项目根目录路径传给代码（SimulationLog 用于固定 log/ 输出位置，Debug/Release/各构建目录一致）
add_compile_definitions(PROJECT_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

# Qt6 仅 GUI 目标需要；控制台程序（icicle/beam/compare）不依赖 Qt
find_package(Qt6 COMPONENTS Core Widgets)
set(BUILD_GUI FALSE)
if(Qt6_FOUND)
    # Qt 分为 MSVC 版与 MinGW 版，二者不能混用。若 MinGW/GCC 构建时误找到
    # MSVC 版 Qt，GUI 编译会失败，此时自动跳过 GUI，仅构建控制台程序。
    if(NOT MSVC AND Qt6_DIR MATCHES "msvc")
        message(WARNING "Qt6 found at ${Qt6_DIR} is an MSVC build, incompatible with the current compiler. "
                        "GUI target will be skipped; set CMAKE_PREFIX_PATH to a MinGW/GCC Qt to build the GUI.")
    elseif(NOT MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)
        # Qt 6.10 起大量头文件使用 Q_DECLARE_*_NON_NOEXCEPT 宏，内部 static_assert 要求编译器
        # 支持相应 __cpp_lib_* 特性宏，GCC 11 以下会直接编译失败（报 "Use *_NON_NOEXCEPT version of the macro"）。
        # 控制台程序不依赖 Qt，因此这里只跳过 GUI，而不是让整个构建失败。
        message(WARNING "GCC ${CMAKE_CXX_COMPILER_VERSION} is too old for Qt ${Qt6_VERSION} GUI headers "
                        "(needs GCC 11+). GUI target will be skipped; console targets build normally. "
                        "Use MSVC or a GCC 11+ toolchain to build the GUI.")
    else()
        set(BUILD_GUI TRUE)
        message(STATUS "Found Qt6: ${Qt6_DIR}")
    endif()
else()
    message(WARNING "Qt6 not found (可设置 CMAKE_PREFIX_PATH 指向 Qt 安装目录)。GUI 目标将跳过，仅构建控制台程序。")
endif()

# 头文件路径（也可改用 target_include_directories 更精细地控制）
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    add_compile_options(/utf-8)
endif()

# 公共核心源文件
set(CORE_SOURCES
    ./core/Action.cpp
    ./core/AutoAttack.cpp
    ./core/Buff.cpp
    ./core/Info.cpp
    ./core/Person.cpp
    ./core/Skill.cpp
    ./core/Logger.cpp
    ./core/Statistics.cpp
    ./core/Comparison.cpp
    ./core/SimulationLog.cpp
    ./FightingFantasy/Skill.cpp
    ./FightingFantasy/Buff.cpp
)

# 冰矛职业实现
set(ICICLE_SOURCES
    ./Mage/Icicle/Skill.cpp
    ./Mage/Icicle/Person.cpp
    ./Mage/Icicle/Buff.cpp
    ./Mage/Icicle/AutoAttack.cpp
    ./Mage/Icicle/Action.cpp
)

# 射线职业实现
set(BEAM_SOURCES
    ./Mage/Beam/Skill.cpp
    ./Mage/Beam/Person.cpp
    ./Mage/Beam/Buff.cpp
    ./Mage/Beam/AutoAttack.cpp
    ./Mage/Beam/Action.cpp
)

# GUI 源文件
set(GUI_SOURCES
    main.cpp
    mainwindow.cpp
)

# ===== 1. 创建核心静态库 =====
add_library(core_lib STATIC ${CORE_SOURCES})

# 为库设置编译选项（沿用原可执行文件的通用选项）
# 这些选项为 MSVC 专用，GCC/Clang 无法识别，因此必须包在 if(MSVC) 中
if(MSVC)
    target_compile_options(core_lib PRIVATE
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
        $<$<CONFIG:MinSizeRel>:/O1>
        /utf-8
    )
endif()

# ===== 2. 创建冰矛控制台程序 =====
add_executable(dps_simulator_icicle ${ICICLE_SOURCES} ./main_icicle.cpp)
target_link_libraries(dps_simulator_icicle PRIVATE core_lib)
set_target_properties(dps_simulator_icicle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ===== 3. 创建射线控制台程序 =====
add_executable(dps_simulator_beam ${BEAM_SOURCES} ./main_beam.cpp)
target_link_libraries(dps_simulator_beam PRIVATE core_lib)
set_target_properties(dps_simulator_beam PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ===== 3.1 射线各场景技能伤害峰值统计 =====
add_executable(dps_simulator_beam_max ${BEAM_SOURCES} ./main_beam_max.cpp)
target_link_libraries(dps_simulator_beam_max PRIVATE core_lib)
set_target_properties(dps_simulator_beam_max PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ===== 4. 创建 GUI 程序（需要 Qt6）=====
# GUI 需要包含所有职业的实现源文件
if(BUILD_GUI)
    add_executable(dps_simulator_gui ${ICICLE_SOURCES} ${BEAM_SOURCES} ${GUI_SOURCES})
    target_link_libraries(dps_simulator_gui PRIVATE core_lib Qt6::Core Qt6::Widgets Qt6::Gui)
    set_target_properties(dps_simulator_gui PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    if(MSVC)
        target_compile_options(dps_simulator_gui PRIVATE /permissive- /Zc:__cplusplus /wd4244 /wd4267)
    endif()
endif()

# ===== 5. 创建配对对比程序（CRN 同种子） =====
# 对比"基准配置"与多个"候选配置"的提升，见 core/Comparison.h
add_executable(dps_simulator_compare ${ICICLE_SOURCES} ${BEAM_SOURCES} ./main_compare.cpp)
target_link_libraries(dps_simulator_compare PRIVATE core_lib)
set_target_properties(dps_simulator_compare PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ===== 6. 自动化回归测试 =====
# enable_testing() 打开 CMake 的测试功能；add_test 把可执行文件登记成一个用例，
# 之后用 ctest -C Release 就能一次跑完并汇总。ctest 只看**进程退出码**：
# 0=通过，非 0=失败（见 tests/test_main.cpp 的 return）。
enable_testing()

# 测试拆成两个独立可执行文件，而不是挤进一个：
#   · 单元测试碰属性构造（快，毫秒级）
#   · 黄金基线跑完整 180 秒模拟（慢，十几秒）
# Person/Action 系列里有大量**进程级 static**（TypedAction<...>::listeners、
# Buff::ID、AutoAttack::timer），拆成两个进程是最彻底的隔离方式，
# 顺带让 ctest 的报告能直接指认是"数值单元测试"还是"黄金基线"挂了。
add_executable(dps_unit_tests
    tests/test_attributes.cpp
    tests/test_main.cpp
    ${BEAM_SOURCES}
)
target_link_libraries(dps_unit_tests PRIVATE core_lib)
set_target_properties(dps_unit_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
# 用 $<TARGET_FILE:...> 而不是裸目标名：多配置生成器（Visual Studio）下
# 这样才能正确定位到 bin/Release/xxx.exe，不会因为找不到文件而报错。
add_test(NAME unit_tests COMMAND $<TARGET_FILE:dps_unit_tests>)

add_executable(dps_golden_tests
    tests/test_golden_beam.cpp
    tests/test_parallel_isolation.cpp
    tests/test_main.cpp
    ${BEAM_SOURCES}
)
target_link_libraries(dps_golden_tests PRIVATE core_lib)
set_target_properties(dps_golden_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
add_test(NAME golden_beam COMMAND $<TARGET_FILE:dps_golden_tests>)