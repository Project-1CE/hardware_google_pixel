/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "adaptivecpu/CpuLoadReaderSysDevices.h"
#include "mocks.h"

#define LOG_TAG "powerhal-libperfmgr"

using testing::_;
using testing::ByMove;
using testing::MatchesRegex;
using testing::Return;
using std::chrono_literals::operator""ms;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

TEST(CpuLoadReaderSysDevicesTest, GetRecentCpuLoads) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    std::unique_ptr<MockTimeSource> timeSource = std::make_unique<MockTimeSource>();
    EXPECT_CALL(*filesystem, listDirectory(MatchesRegex("/sys/devices/system/cpu/cpu0/cpuidle")))
            .WillOnce(Return(std::vector<std::string>{"foo", "bar", "baz"}));
    EXPECT_CALL(*filesystem,
                listDirectory(MatchesRegex("/sys/devices/system/cpu/cpu0/cpuidle/(foo|bar)")))
            .Times(2)
            .WillRepeatedly(Return(std::vector<std::string>{"abc", "time", "xyz"}));
    EXPECT_CALL(*filesystem,
                listDirectory(MatchesRegex("/sys/devices/system/cpu/cpu0/cpuidle/baz")))
            .WillOnce(Return(std::vector<std::string>{"abc", "xyz"}));

    EXPECT_CALL(*filesystem, readFileStream("/sys/devices/system/cpu/cpu0/cpuidle/foo/time"))
            .Times(2)
            .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("100"))))
            .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("200"))));
    EXPECT_CALL(*filesystem, readFileStream("/sys/devices/system/cpu/cpu0/cpuidle/bar/time"))
            .Times(2)
            .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("500"))))
            .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("700"))));

    EXPECT_CALL(*filesystem, readFileStream("/sys/devices/system/cpu/cpu1/cpuidle/foo/time"))
            .Times(2)
            .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("1000"))))
            .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("1010"))));
    EXPECT_CALL(*filesystem, readFileStream("/sys/devices/system/cpu/cpu1/cpuidle/bar/time"))
            .Times(2)
            .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("50"))))
            .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("70"))));

    EXPECT_CALL(
            *filesystem,
            readFileStream(MatchesRegex("/sys/devices/system/cpu/cpu[2-7]/cpuidle/(foo|bar)/time")))
            .WillRepeatedly([]() { return std::make_unique<std::istringstream>("0"); });

    EXPECT_CALL(*timeSource, GetTime()).Times(2).WillOnce(Return(1ms)).WillOnce(Return(2ms));

    CpuLoadReaderSysDevices reader(std::move(filesystem), std::move(timeSource));
    ASSERT_TRUE(reader.Init());

    std::array<double, NUM_CPU_CORES> actualPercentage;
    reader.GetRecentCpuLoads(&actualPercentage);

    std::array<double, NUM_CPU_CORES> expectedPercentage{0.3, 0.03, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(actualPercentage, expectedPercentage);
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
