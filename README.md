# SaveGSystem Plugin for Unreal Engine  
  
The **SaveGSystem** plugin is a robust and flexible save/load system designed for Unreal Engine 5.5. It provides a comprehensive framework for saving and loading game data, including actors, properties, and complex data structures. The plugin is highly customizable and supports both synchronous and asynchronous operations, making it suitable for a wide range of game development scenarios.  
**Note**: This plugin is designed exclusively for **single-player games** and **does not support networking or multiplayer functionality**. If you are developing a multiplayer game, you will need to implement additional networking logic to handle save/load operations across clients and servers.  


## Features

- **Save and Load Game Data**: Easily save and load game data for actors and objects that implement the `USaveGInterface`.
- **Asynchronous Operations**: Supports asynchronous save and load operations to avoid blocking the main game thread.
- **Data Compression**: Utilizes Zlib compression to reduce the size of saved data.
- **JSON Support**: Optionally save data in JSON format for easy debugging and manual editing.
- **Customizable Metadata**: Mark properties with `SaveGame` metadata to control which properties are saved.
- **Support for Complex Data Types**: Handles a wide range of data types, including:
    - Primitive types (bool, int, float, etc.)
    - Strings, Names, and Text
    - Enums
    - Structs
    - Arrays
    - Maps
    - Soft Object and Class References
- **Automated Testing**: Includes a suite of automated tests to ensure the reliability of the save/load system.


## Usage

### 1. Implementing the SaveGInterface
   To enable saving and loading for an actor or object, implement the USaveGInterface in your class. This interface provides four methods that allow you to perform actions before and after saving or loading:
   
```c++
class YOURGAME_API AYourActor : public AActor, public ISaveGInterface
{
    GENERATED_BODY()

public:
    virtual void PreSave_Implementation() override;
    virtual void PostSave_Implementation() override;
    virtual void PreLoad_Implementation() override;
    virtual void PostLoad_Implementation() override;
};
```
### 2. Marking Properties for Saving
Use the `SaveGame` metadata to mark properties that should be saved:
```c++
UCLASS()
class YOURGAME_API AYourActor : public AActor, public ISaveGInterface
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    int32 Health;

    UPROPERTY(SaveGame)
    FString PlayerName;
};
```
### 3. Saving and Loading Data
   Use the `USaveGSubSystem` to save and load data. The subsystem provides methods for saving and loading individual objects, as well as entire worlds.

#### Saving Data
```c++
USaveGSubSystem* SaveSubSystem = USaveGSubSystem::Get(GetWorld());
if (SaveSubSystem)
{
    SaveSubSystem->UpdateSaveData("PlayerData", this);
    SaveSubSystem->SaveDataInFile("MySaveFile");
}
```
#### Loading Data
```c++
USaveGSubSystem* SaveSubSystem = USaveGSubSystem::Get(GetWorld());
if (SaveSubSystem)
{
    SaveSubSystem->LoadDataFromFile("MySaveFile");
    SaveSubSystem->LoadSaveData("PlayerData", this);
}
```
### 4. Customizing Save Settings
   You can customize the save system's behavior by modifying the USaveGSettings class. For example, you can enable or disable saving data in JSON format:
```c++
UCLASS(Config = "Game", defaultconfig, meta = (DisplayName = "SaveG System Settings"))
class SAVEGSYSTEM_API USaveGSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "General Settings")
    bool bEnableSaveDataJSONFile{false};
};
```
### 5. Running Automated Tests
   The plugin includes a suite of automated tests to ensure the save/load system works as expected. You can run these tests from the Unreal Engine editor:

Open the `Session Frontend` (`Window > Developer Tools > Session Frontend`).

Go to the `Automation` tab.

Run the tests under the `SaveGSystem` category.


## Example
Here’s a simple example of how to use the SaveGSystem in your game:

```c++
// In YourActor.h
UCLASS()
class YOURGAME_API AYourActor : public AActor, public ISaveGInterface
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    int32 Health;

    UPROPERTY(SaveGame)
    FString PlayerName;

    virtual void PreSave_Implementation() override
    {
        // Perform any pre-save actions here
    }

    virtual void PostSave_Implementation() override
    {
        // Perform any post-save actions here
    }

    virtual void PreLoad_Implementation() override
    {
        // Perform any pre-load actions here
    }

    virtual void PostLoad_Implementation() override
    {
        // Perform any post-load actions here
    }
};

// In YourGameMode.cpp
void AYourGameMode::SaveGame()
{
    USaveGSubSystem* SaveSubSystem = USaveGSubSystem::Get(GetWorld());
    if (SaveSubSystem)
    {
        SaveSubSystem->UpdateSaveData("PlayerData", PlayerActor);
        SaveSubSystem->SaveDataInFile("MySaveFile");
    }
}

void AYourGameMode::LoadGame()
{
    USaveGSubSystem* SaveSubSystem = USaveGSubSystem::Get(GetWorld());
    if (SaveSubSystem)
    {
        SaveSubSystem->LoadDataFromFile("MySaveFile");
        SaveSubSystem->LoadSaveData("PlayerData", PlayerActor);
    }
}
```


## 📫 Other <a name="Other"></a>
:bangbang: Attention: If you can improve my trash code then make a pull request.

**:copyright:Authors:**

*[Mark Veligod](https://github.com/markveligod)*  
