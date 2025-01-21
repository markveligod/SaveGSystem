
#include "SaveGSystem/Tests/SaveGSystemTests.h"

#include "SaveGSystem/Library/SaveGLibrary.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemArchiveBoolTest, "SaveGSystem.Archive.BoolTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArchiveBoolTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestBoolObject>("TestBoolObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemArchiveByteTest, "SaveGSystem.Archive.ByteTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArchiveByteTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestByteObject>("TestByteObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemArchiveNumericTest, "SaveGSystem.Archive.NumericTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArchiveNumericTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestNumericObject>("TestNumericObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemArchiveStringTest, "SaveGSystem.Archive.StringTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArchiveStringTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestStringObject>("TestStringObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemArchiveObjectTest, "SaveGSystem.Archive.ObjectTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArchiveObjectTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestObjectHandle>("TestObjectHandle", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemArchiveStructTest, "SaveGSystem.Archive.StructTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArchiveStructTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestStructObject>("TestStructObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveGSystemArchiveArrayStructTest, "SaveGSystem.Archive.ArrayStructTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArchiveArrayStructTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestArrayStructObject>("TestArrayStructObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemArchiveMapTest, "SaveGSystem.Archive.MapTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSaveGSystemArchiveMapTest::RunTest(const FString& Parameters)
{
    return SaveGSystemTests::RunSaveGSystemTest<USaveGTestMapStructObject>("TestMapStructObject", this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemConvertToString, "SaveGSystem.Convert.ToString", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaveGSystemConvertToString::RunTest(const FString& Parameters)
{
    // Create a JSON object
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("Key1"), TEXT("Value1"));
    JsonObject->SetNumberField(TEXT("Key2"), 42);
    JsonObject->SetBoolField(TEXT("Key3"), true);

    // Convert JSON object to string
    FString JsonString = USaveGLibrary::ConvertJsonObjectToString(JsonObject);

    // Expected JSON string
    FString ExpectedJsonString = TEXT("{\r\n\t\"Key1\": \"Value1\",\r\n\t\"Key2\": 42,\r\n\t\"Key3\": true\r\n}");

    // Test if the conversion is correct
    TestEqual(TEXT("ConvertJsonObjectToString should return the correct JSON string"), JsonString, ExpectedJsonString);

    // Test with an invalid JSON object
    TSharedPtr<FJsonObject> InvalidJsonObject = nullptr;
    FString InvalidJsonString = USaveGLibrary::ConvertJsonObjectToString(InvalidJsonObject);

    // Test if the function returns an empty string for an invalid JSON object
    TestTrue(TEXT("ConvertJsonObjectToString should return an empty string for an invalid JSON object"), InvalidJsonString.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemConvertToJsonObject, "SaveGSystem.Convert.ToJsonObject", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaveGSystemConvertToJsonObject::RunTest(const FString& Parameters)
{
    // Create a JSON string
    FString JsonString = TEXT("{\r\n\t\"Key1\": \"Value1\",\r\n\t\"Key2\": 42,\r\n\t\"Key3\": true\r\n}");

    // Convert JSON string to object
    TSharedPtr<FJsonObject> JsonObject = USaveGLibrary::ConvertStringToJsonObject(JsonString);

    // Test if the conversion is successful
    TestNotNull(TEXT("ConvertStringToJsonObject should return a valid JSON object"), JsonObject.Get());

    // Test the values in the JSON object
    if (JsonObject.IsValid())
    {
        FString Value1;
        int32 Value2;
        bool Value3;

        TestTrue(TEXT("JsonObject should contain Key1"), JsonObject->TryGetStringField(TEXT("Key1"), Value1));
        TestEqual(TEXT("Key1 should have the correct value"), Value1, TEXT("Value1"));

        TestTrue(TEXT("JsonObject should contain Key2"), JsonObject->TryGetNumberField(TEXT("Key2"), Value2));
        TestEqual(TEXT("Key2 should have the correct value"), Value2, 42);

        TestTrue(TEXT("JsonObject should contain Key3"), JsonObject->TryGetBoolField(TEXT("Key3"), Value3));
        TestEqual(TEXT("Key3 should have the correct value"), Value3, true);
    }

    // Test with an invalid JSON string
    FString InvalidJsonString = TEXT("Invalid JSON String");
    TSharedPtr<FJsonObject> InvalidJsonObject = USaveGLibrary::ConvertStringToJsonObject(InvalidJsonString);

    // Test if the function returns a null pointer for an invalid JSON string
    TestNull(TEXT("ConvertStringToJsonObject should return a null pointer for an invalid JSON string"), InvalidJsonObject.Get());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemZlibCompression, "SaveGSystem.Zlib.Compression", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaveGSystemZlibCompression::RunTest(const FString& Parameters)
{
    // Create a test FString
    FString TestString;
    for (int32 i = 0; i < 100; i++)
    {
        TestString.Append(TEXT("This is a test string for compression. 🚀") + FString::FromInt(i) + static_cast<TCHAR>(FMath::RandRange(0, UINT8_MAX)));
    }

    // Convert the FString to a byte array
    TArray<uint8> OriginalData = USaveGLibrary::ConvertStringToByte(TestString);
    TestTrue(TEXT("OriginalData should not be empty"), OriginalData.Num() > 0);

    // Compress the byte array
    TArray<uint8> CompressedData;
    bool bCompressSuccess = USaveGLibrary::CompressData(OriginalData, CompressedData);
    TestTrue(TEXT("CompressData should succeed"), bCompressSuccess);
    TestTrue(TEXT("CompressedData should not be empty"), CompressedData.Num() > 0);

    // Verify that the compressed data is smaller than the original data (or equal, if compression is not effective)
    TestTrue(TEXT("CompressedData should be smaller than or equal to OriginalData"), CompressedData.Num() <= OriginalData.Num());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveGSystemZlibDecompression, "SaveGSystem.Zlib.Decompression", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaveGSystemZlibDecompression::RunTest(const FString& Parameters)
{
    // Create a test FString
    FString TestString;
    for (int32 i = 0; i < 100; i++)
    {
        TestString.Append(TEXT("This is a test string for decompression. 🚀") + FString::FromInt(i) + static_cast<TCHAR>(FMath::RandRange(0, UINT8_MAX)));
    }

    // Convert the FString to a byte array
    TArray<uint8> OriginalData = USaveGLibrary::ConvertStringToByte(TestString);
    TestTrue(TEXT("OriginalData should not be empty"), OriginalData.Num() > 0);

    // Compress the byte array
    TArray<uint8> CompressedData;
    bool bCompressSuccess = USaveGLibrary::CompressData(OriginalData, CompressedData);
    TestTrue(TEXT("CompressData should succeed"), bCompressSuccess);
    TestTrue(TEXT("CompressedData should not be empty"), CompressedData.Num() > 0);

    // Decompress the byte array
    TArray<uint8> DecompressedData;
    bool bDecompressSuccess = USaveGLibrary::DecompressData(CompressedData, DecompressedData);
    TestTrue(TEXT("DecompressData should succeed"), bDecompressSuccess);
    TestTrue(TEXT("DecompressedData should not be empty"), DecompressedData.Num() > 0);

    // Convert the decompressed byte array back to an FString
    FString DecompressedString = USaveGLibrary::ConvertByteToString(DecompressedData);
    TestEqual(TEXT("DecompressedString should match TestString"), DecompressedString, TestString);

    return true;
}
#endif