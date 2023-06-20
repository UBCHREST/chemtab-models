#include "eos/chemTabTestFixture.hpp"
#include "gtest/gtest.h"
#include "localPath.hpp"
#include "modelPath.hpp"

/**
 * Add a test to make sure that chemTab is available
 */
TEST(ChemTabRepoTests, ShouldOnlyPassIfTensflowIsFound) {
#ifndef WITH_TENSORFLOW
    FAIL() << "Should be tested with TensorFlow";
#else
    SUCCEED() << "TensorFlow Found";
#endif
}

/**
 * This helper lambda gets a list of models from the specified modelPath
 */
std::function<std::vector<ChemTabTestParameters>()> buildTestParameters = []() {
    std::vector<ChemTabTestParameters> testParameters;

    // list all directories in the CHEMTAB_MODEL_PATH
    for (const auto &entry : std::filesystem::directory_iterator(CHEMTAB_MODEL_PATH)) {
        // check to see if it is a directory
        if (entry.is_directory()) {
            // check to see if it contains a test target
            auto testTarget = entry.path() / "testTargets.yaml";
            if (std::filesystem::exists(testTarget)) {
                // add to the test parameters
                testParameters.push_back((ChemTabTestParameters){.modelPath = absolute(entry.path()), .testTargetFile = absolute(testTarget)});
            }
        }
    }

    return testParameters;
};

/**
 * Build the list of test parameters from the provided lambda
 * @return
 */
INSTANTIATE_TEST_SUITE_P(ChemTabRepoTests, ChemTabTestFixture, testing::ValuesIn(buildTestParameters()), [](const testing::TestParamInfo<ChemTabTestParameters> &info) {
    auto testName = info.param.modelPath.filename().string();
    auto it = std::remove_if(testName.begin(), testName.end(), [](char const &c) { return !std::isalnum(c); });

    testName.erase(it, testName.end());
    return testName;
});

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // Only run the chemTabTests
    ::testing::GTEST_FLAG(filter) = "ChemTabRepoTests*";
    return RUN_ALL_TESTS();
}