cmake_minimum_required(VERSION 3.16)
project(dps_simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

# Qt6 仅 GUI 目标需要；控制台程序（icicle/beam/compare）不依赖 Qt
find_package(Qt6 COMPONENTS Core Widgets)
if(Qt6_FOUND)
    message(STATUS "Found Qt6: ${Qt6_DIR}")
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
target_compile_options(core_lib PRIVATE
    $<$<CONFIG:Debug>:/Od /Zi /RTC1>
    $<$<CONFIG:Release>:/O2>
    $<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
    $<$<CONFIG:MinSizeRel>:/O1>
)
if(MSVC)
    target_compile_options(core_lib PRIVATE /utf-8)
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

# ===== 4. 创建 GUI 程序（需要 Qt6）=====
# GUI 需要包含所有职业的实现源文件
if(Qt6_FOUND)
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