
#include "SaveGSystem/Tests/SaveGSystemTests.h"
#include "SaveGSystem/SubSystem/SaveGSubSystem.h"

#if WITH_AUTOMATION_TESTS

namespace SaveGSystemTests
{
template <typename ObjectType>
bool RunSaveGSystemTest(const FString& TagName, FAutomationTestBase* Test)
{
    FInitTestWorld TestWorld;
    if (!Test->TestTrue(TEXT("TestWorld is not valid"), TestWorld.IsValid())) return false;

    ObjectType* TestObject = NewObject<ObjectType>(TestWorld.WeakWorld.Get());
    if (!Test->TestNotNull(TEXT("TestObject is nullptr"), TestObject)) return false;

    TestObject->Generate();
    TestWorld.WeakSaveGSubSystem->UpdateSaveData(TagName, TestObject);

    FWorldSimulationTicker SavedSimulateTick([TestObject]() { return TestObject->IsSaved(); }, 6.0f, TestWorld.WeakWorld.Get());
    SavedSimulateTick.Run();

    if (!Test->TestTrue(TEXT("TestObject is not saved in subsystem"), TestObject->IsSaved())) return false;
    if (!Test->TestTrue(TEXT("TestObject is not saved in subsystem"), TestWorld.WeakSaveGSubSystem->IsHaveTag(TagName))) return false;

    TestObject->Reset();
    TestWorld.WeakSaveGSubSystem->LoadSaveData(TagName, TestObject);
    FWorldSimulationTicker LoadedSimulateTick([TestObject]() { return TestObject->IsLoaded(); }, 6.0f, TestWorld.WeakWorld.Get());
    LoadedSimulateTick.Run();

    if (!Test->TestTrue(TEXT("TestObject is not loaded in subsystem"), TestObject->IsLoaded())) return false;
    if (!Test->TestTrue(TEXT("TestObject is not loaded in subsystem"), TestObject->IsValidValue())) return false;

    return true;
}
}  // namespace SaveGSystemTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemBoolTest, "SaveGSystem.BoolTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemBoolTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestBoolObject>("TestBoolObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemByteTest, "SaveGSystem.ByteTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemByteTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestByteObject>("TestByteObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemNumericTest, "SaveGSystem.NumericTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemNumericTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestNumericObject>("TestNumericObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemStringTest, "SaveGSystem.StringTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemStringTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestStringObject>("TestStringObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemObjectTest, "SaveGSystem.ObjectTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemObjectTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestObjectHandle>("TestObjectHandle", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemStructTest, "SaveGSystem.StructTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemStructTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestStructObject>("TestStructObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemArrayStructTest, "SaveGSystem.ArrayStructTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArrayStructTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestArrayStructObject>("TestArrayStructObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemMapTest, "SaveGSystem.MapTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemMapTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestMapStructObject>("TestMapStructObject", this);
}

#endif