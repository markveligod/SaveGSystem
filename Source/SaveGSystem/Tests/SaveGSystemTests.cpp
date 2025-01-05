
#include "SaveGSystem/Tests/SaveGSystemTests.h"
#include "SaveGSystem/SubSystem/SaveGSubSystem.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemBoolTest, "SaveGSystem.BoolTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemNumericTest, "SaveGSystem.NumericTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaveGSystemBoolTest::RunTest(const FString& Parameters)
{
    FInitTestWorld TestWorld;
    if (!TestTrue(TEXT("TestWorld is not valid"), TestWorld.IsValid())) return false;

    USaveGTestBoolObject* TestBoolObject = NewObject<USaveGTestBoolObject>(TestWorld.WeakWorld.Get());
    if (!TestNotNull("TestBoolObject is nullptr", TestBoolObject)) return false;

    TestBoolObject->bTestBool = true;
    TestWorld.WeakSaveGSubSystem->UpdateSaveData("TestBoolObject", TestBoolObject);

    FWorldSimulationTicker SavedSimulateTick([TestBoolObject]()
    {
        return TestBoolObject->IsSaved();
    }, 6.0f, TestWorld.WeakWorld.Get());
    SavedSimulateTick.Run();

    if (!TestTrue(TEXT("TestBoolObject is not saved in subsystem"), TestBoolObject->IsSaved())) return false;
    if (!TestTrue(TEXT("TestBoolObject is not saved in subsystem"), TestWorld.WeakSaveGSubSystem->IsHaveTag("TestBoolObject"))) return false;

    TestBoolObject->bTestBool = false;
    TestWorld.WeakSaveGSubSystem->LoadSaveData("TestBoolObject", TestBoolObject);
    FWorldSimulationTicker LoadedSimulateTick([TestBoolObject]()
    {
        return TestBoolObject->IsLoaded();
    }, 6.0f, TestWorld.WeakWorld.Get());
    LoadedSimulateTick.Run();

    if (!TestTrue(TEXT("TestBoolObject is not loaded in subsystem"), TestBoolObject->IsLoaded())) return false;
    if (!TestTrue(TEXT("TestBoolObject is not loaded in subsystem"), TestBoolObject->bTestBool)) return false;

    return true;
}

bool FSaveGSystemNumericTest::RunTest(const FString& Parameters)
{
    FInitTestWorld TestWorld;
    if (!TestTrue(TEXT("TestWorld is not valid"), TestWorld.IsValid())) return false;

    USaveGTestNumericObject* TestNumericObject = NewObject<USaveGTestNumericObject>(TestWorld.WeakWorld.Get());
    if (!TestNotNull("TestNumericObject is nullptr", TestNumericObject)) return false;

    TestNumericObject->GenerateNumeric();
    TestWorld.WeakSaveGSubSystem->UpdateSaveData("TestNumericObject", TestNumericObject);

    FWorldSimulationTicker SavedSimulateTick([TestNumericObject]()
    {
        return TestNumericObject->IsSaved();
    }, 6.0f, TestWorld.WeakWorld.Get());
    SavedSimulateTick.Run();

    if (!TestTrue(TEXT("TestNumericObject is not saved in subsystem"), TestNumericObject->IsSaved())) return false;
    if (!TestTrue(TEXT("TestNumericObject is not saved in subsystem"), TestWorld.WeakSaveGSubSystem->IsHaveTag("TestNumericObject"))) return false;

    TestNumericObject->ResetNumeric();
    TestWorld.WeakSaveGSubSystem->LoadSaveData("TestNumericObject", TestNumericObject);
    FWorldSimulationTicker LoadedSimulateTick([TestNumericObject]()
    {
        return TestNumericObject->IsLoaded();
    }, 6.0f, TestWorld.WeakWorld.Get());
    LoadedSimulateTick.Run();

    if (!TestTrue(TEXT("TestNumericObject is not loaded in subsystem"), TestNumericObject->IsLoaded())) return false;
    if (!TestTrue(TEXT("TestNumericObject is not loaded in subsystem"), TestNumericObject->IsHaveNoneZeroValue())) return false;
    return true;
}
#endif