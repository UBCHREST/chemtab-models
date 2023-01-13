#include <yaml-cpp/yaml.h>
#include "PetscTestFixture.hpp"
#include "eos/chemTab.hpp"
#include "finiteVolume/compressibleFlowFields.hpp"
#include "gtest/gtest.h"
#include "localPath.hpp"
#include "modelPath.hpp"

/*******************************************************************************************************
 * Tests for expected input/outputs
 */
struct ChemTabModelTestParameters {
    std::filesystem::path modelPath;
    std::filesystem::path testTargetFile;
};

class ChemTabModelTestFixture : public testingResources::PetscTestFixture, public testing::WithParamInterface<ChemTabModelTestParameters> {
   protected:
    YAML::Node testTargets;

    void SetUp() override {
        testingResources::PetscTestFixture::SetUp();
        testTargets = YAML::LoadFile(GetParam().testTargetFile);

        // this should be an array
        if (!testTargets.IsSequence()) {
            FAIL() << "The provided test targets " + GetParam().testTargetFile.string() + " must be an sequence.";
        }
    }
};

TEST_P(ChemTabModelTestFixture, ShouldReturnCorrectSpeciesAndVariables) {
    // iterate over each test
    for (const auto &testTarget : testTargets) {
        // arrange
        ablate::eos::ChemTab chemTabModel(GetParam().modelPath);

        // act
        auto actualSpeciesVariables = chemTabModel.GetSpeciesVariables();
        auto actualSpecies = chemTabModel.GetSpecies();
        auto actualProgressVariables = chemTabModel.GetProgressVariables();

        // assert
        EXPECT_TRUE(actualSpeciesVariables.empty()) << "should report no transport species" << testTarget["testName"].as<std::string>();
        EXPECT_EQ(testTarget["species_names"].as<std::vector<std::string>>(), actualSpecies) << "should compute correct species name for model " << testTarget["testName"].as<std::string>();
        EXPECT_EQ(testTarget["cpv_names"].as<std::vector<std::string>>(), actualProgressVariables) << "should compute correct cpv names for model " << testTarget["testName"].as<std::string>();
        EXPECT_EQ(testTarget["species_names"].as<std::vector<std::string>>().size(), actualSpecies.size())
            << "should compute correct species name for model " << testTarget["testName"].as<std::string>();
    }
}

#define assert_float_close(expected, actual) EXPECT_NEAR(expected, actual, PetscAbs(1.0E-5 * actual))  // gives you relative error check

/*******************************************************************************************************
 * Tests for getting the Compute Mass Fractions Functions
 */
TEST_P(ChemTabModelTestFixture, ShouldComputeCorrectMassFractions) {
    ;

    // iterate over each test
    for (const auto &testTarget : testTargets) {
        // arrange
        ablate::eos::ChemTab chemTabModel(GetParam().modelPath);
        auto expectedMassFractions = testTarget["output_mass_fractions"].as<std::vector<double>>();
        auto inputProgressVariables = testTarget["input_cpvs"].as<std::vector<double>>();

        // act
        std::vector<PetscReal> actual(expectedMassFractions.size());
        chemTabModel.ComputeMassFractions(inputProgressVariables.data(), inputProgressVariables.size(), actual.data(), actual.size());

        // assert
        for (std::size_t r = 0; r < actual.size(); r++) {
            assert_float_close(expectedMassFractions[r], actual[r]) << "The value for [" << r << "] is incorrect for model " << testTarget["testName"].as<std::string>();
        }
    }
}

/*******************************************************************************************************
 * Tests for getting the Source and Source Energy Predictions
 */
TEST_P(ChemTabModelTestFixture, ShouldComputeCorrectSource) {
    // iterate over each test
    for (const auto &testTarget : testTargets) {
        // arrange
        ablate::eos::ChemTab chemTabModel(GetParam().modelPath);
        auto inputProgressVariables = testTarget["input_cpvs"].as<std::vector<double>>();
        auto expectedSourceEnergy = testTarget["output_source_energy"].as<double>();
        auto expectedSourceProgress = testTarget["output_source_terms"].as<std::vector<double>>();

        // assume a density
        PetscReal density = 1.5;

        // size up and set the expected input
        std::vector<PetscReal> conservedProgressVariable(chemTabModel.GetProgressVariables().size(), 0.0);
        for (std::size_t p = 0; p < inputProgressVariables.size(); p++) {
            conservedProgressVariable[p] = inputProgressVariables[p] * density;
        }

        // act
        // Size up the results based upon expected
        std::vector<PetscReal> actualSourceProgress(conservedProgressVariable.size(), 0.0);
        PetscReal actualSourceEnergy = 0.0;
        chemTabModel.ChemistrySource(density, conservedProgressVariable.data(), &actualSourceEnergy, actualSourceProgress.data());

        assert_float_close(expectedSourceEnergy, actualSourceEnergy) << "The sourceEnergy is incorrect for model " << testTarget["testName"].as<std::string>();

        for (std::size_t r = 0; r < expectedSourceProgress.size(); r++) {
            assert_float_close(expectedSourceProgress[r], actualSourceProgress[r]) << " the percent difference of (" << expectedSourceProgress[r] << ", " << actualSourceProgress[r]
                                                                                   << ") should be less than 5.0E-6 for index [" << r << "] for model " << testTarget["testName"].as<std::string>();
        }
    }
}

TEST_P(ChemTabModelTestFixture, ShouldComputeCorrectThermalProperties) {
    // iterate over each test
    for (const auto &testTarget : testTargets) {
        // ARRANGE
        auto chemTab = std::make_shared<ablate::eos::ChemTab>(GetParam().modelPath);
        auto expectedMassFractions = testTarget["output_mass_fractions"].as<std::vector<double>>();
        auto inputProgressVariables = testTarget["input_cpvs"].as<std::vector<double>>();
        auto inputProgressVariablesNames = testTarget["cpv_names"].as<std::vector<std::string>>();

        // build a new reference eos
        auto metadata = YAML::LoadFile(std::filesystem::path(GetParam().modelPath) / "metadata.yaml");
        auto tchem = std::make_shared<ablate::eos::TChem>(std::filesystem::path(GetParam().modelPath) / metadata["mechanism"].as<std::string>());

        // assume values for density and energy
        double density = 1.2;
        double densityEnergy = 1.2 * 1.0E+05;
        double momentum = 1.2 * 10;

        // build a conserved array for only euler
        std::vector<PetscReal> eulerConserved = {0.0, density, densityEnergy, momentum};
        std::vector<PetscReal> allFieldsConserved = eulerConserved;
        for (auto pv : inputProgressVariables) {
            allFieldsConserved.push_back(pv * density);
        }

        // create fake fields for testings
        auto fields = {ablate::domain::Field{.name = ablate::finiteVolume::CompressibleFlowFields::EULER_FIELD, .numberComponents = 3, .offset = 1},
                       ablate::domain::Field{.name = ablate::finiteVolume::CompressibleFlowFields::DENSITY_PROGRESS_FIELD,
                                             .numberComponents = (PetscInt)inputProgressVariablesNames.size(),
                                             .components = inputProgressVariablesNames,
                                             .offset = (PetscInt)eulerConserved.size()}};

        // compute the reference temperature for other calculations
        auto tChemTemperatureFunction = tchem->GetThermodynamicMassFractionFunction(ablate::eos::ThermodynamicProperty::Temperature, fields);
        PetscReal tChemComputedTemperature;
        ASSERT_EQ(tChemTemperatureFunction.function(eulerConserved.data(), expectedMassFractions.data(), &tChemComputedTemperature, tChemTemperatureFunction.context.get()), 0);
        auto chemTabTemperatureFunction = chemTab->GetThermodynamicFunction(ablate::eos::ThermodynamicProperty::Temperature, fields);
        PetscReal chemTabComputedTemperature;
        ASSERT_EQ(chemTabTemperatureFunction.function(allFieldsConserved.data(), &chemTabComputedTemperature, chemTabTemperatureFunction.context.get()), 0);
        ASSERT_FLOAT_EQ(chemTabComputedTemperature, tChemComputedTemperature) << " The TChem and ChemTab temperatures should be equal";

        // Now check the other thermodynamic properties
        auto testProperties = {ablate::eos::ThermodynamicProperty::Pressure,
                               ablate::eos::ThermodynamicProperty::Temperature,
                               ablate::eos::ThermodynamicProperty::InternalSensibleEnergy,
                               ablate::eos::ThermodynamicProperty::SensibleEnthalpy,
                               ablate::eos::ThermodynamicProperty::SpecificHeatConstantVolume,
                               ablate::eos::ThermodynamicProperty::SpecificHeatConstantPressure,
                               ablate::eos::ThermodynamicProperty::SpeedOfSound,
                               ablate::eos::ThermodynamicProperty::Density};

        for (auto &testProperty : testProperties) {
            PetscReal tChemComputedProperty, chemTabComputedProperty;

            // Test the direction function
            auto tChemFunction = tchem->GetThermodynamicMassFractionFunction(testProperty, fields);
            ASSERT_EQ(tChemFunction.function(eulerConserved.data(), expectedMassFractions.data(), &tChemComputedProperty, tChemFunction.context.get()), 0);

            auto chemTabFunction = chemTab->GetThermodynamicFunction(testProperty, fields);
            ASSERT_EQ(chemTabFunction.function(allFieldsConserved.data(), &chemTabComputedProperty, chemTabFunction.context.get()), 0);

            ASSERT_FLOAT_EQ(tChemComputedProperty, chemTabComputedProperty) << " The TChem and ChemTab " << testProperty << " should be equal";

            // test the function where temperature is an input
            auto tChemFunctionTemperature = tchem->GetThermodynamicTemperatureMassFractionFunction(testProperty, fields);
            ASSERT_EQ(tChemFunctionTemperature.function(eulerConserved.data(), expectedMassFractions.data(), tChemComputedTemperature, &tChemComputedProperty, tChemFunctionTemperature.context.get()),
                      0);

            auto chemTabFunctionTemperature = chemTab->GetThermodynamicTemperatureFunction(testProperty, fields);
            ASSERT_EQ(chemTabFunctionTemperature.function(allFieldsConserved.data(), chemTabComputedTemperature, &chemTabComputedProperty, chemTabFunctionTemperature.context.get()), 0);

            ASSERT_FLOAT_EQ(tChemComputedProperty, chemTabComputedProperty) << " The TChem and ChemTab " << testProperty << " should be equal when computed with temperature";
        }
    }
}

/*******************************************************************************************************
 * Tests for getting the Progress Variables
 */
TEST_P(ChemTabModelTestFixture, ShouldComputeCorrectProgressVariables) {
    for (const auto &testTarget : testTargets) {
        // arrange
        ablate::eos::ChemTab chemTabModel(GetParam().modelPath);
        auto expectedProgressVariables = testTarget["output_cpvs"].as<std::vector<double>>();
        auto inputMassFractions = testTarget["input_mass_fractions"].as<std::vector<double>>();

        // act
        // Size up the results based upon expected
        std::vector<PetscReal> actual(expectedProgressVariables.size());
        chemTabModel.ComputeProgressVariables(inputMassFractions.data(), inputMassFractions.size(), actual.data(), actual.size());

        // assert
        for (std::size_t r = 0; r < actual.size(); r++) {
            assert_float_close(expectedProgressVariables[r], actual[r]) << "The value for input set [" << r << "] is incorrect for model " << testTarget["testName"].as<std::string>();
        }
    }
}

/**
 * This helper lambda gets a list of models from the specified modelPath
 */
std::function<std::vector<ChemTabModelTestParameters>()> buildTestParameters = []() {
    std::vector<ChemTabModelTestParameters> testParameters;

    // list all directories in the CHEMTAB_MODEL_PATH
    for (const auto &entry : std::filesystem::directory_iterator(CHEMTAB_MODEL_PATH)) {
        // check to see if it is a directory
        if (entry.is_directory()) {
            // check to see if it contains a test target
            auto testTarget = entry.path() / "testTargets.yaml";
            if (std::filesystem::exists(testTarget)) {
                // add to the test parameters
                testParameters.push_back((ChemTabModelTestParameters){.modelPath = absolute(entry.path()), .testTargetFile = absolute(testTarget)});
            }
        }
    }

    return testParameters;
};

TEST_P(ChemTabModelTestFixture, ShouldComputeProgressVariablesMassFractionsInterchangeability) {
    GTEST_SKIP() << "Test is not working with ChemTab";

    for (const auto &testTarget : testTargets) {
        // arrange
        auto chemTab = std::make_shared<ablate::eos::ChemTab>(GetParam().modelPath);
        auto expectedProgressVariables = testTarget["output_cpvs"].as<std::vector<double>>();
        auto inputMassFractions = testTarget["input_mass_fractions"].as<std::vector<double>>();

        // act
        // Size up the results based upon expected
        std::vector<PetscReal> actualProgressVariables(expectedProgressVariables.size());
        chemTab->ComputeProgressVariables(inputMassFractions.data(), inputMassFractions.size(), actualProgressVariables.data(), actualProgressVariables.size());

        std::vector<PetscReal> actualMassFractions(inputMassFractions.size());
        chemTab->ComputeMassFractions(actualProgressVariables.data(), actualProgressVariables.size(), actualMassFractions.data(), actualMassFractions.size());

        // assert
        for (std::size_t r = 0; r < actualProgressVariables.size(); r++) {
            assert_float_close(expectedProgressVariables[r], actualProgressVariables[r]) << "The value for input set [" << r << "] is incorrect for model " << testTarget["testName"].as<std::string>();
        }
        for (std::size_t r = 0; r < actualMassFractions.size(); r++) {
            assert_float_close(inputMassFractions[r], actualMassFractions[r]) << "The value for input mass fractions [" << r << "] is incorrect for model " << testTarget["testName"].as<std::string>();
        }
    }
}

/**
 * Build the list of test parameters from the provided lambda
 * @return
 */
INSTANTIATE_TEST_SUITE_P(ChemTabTests, ChemTabModelTestFixture, testing::ValuesIn(buildTestParameters()), [](const testing::TestParamInfo<ChemTabModelTestParameters> &info) {
    auto testName = info.param.modelPath.filename().string();
    auto it = std::remove_if(testName.begin(), testName.end(), [](char const &c) { return !std::isalnum(c); });

    testName.erase(it, testName.end());
    return testName;
});
