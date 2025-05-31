#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Materials/MaterialParameterCollection.h"
#include "WeatherEnvironmentalSystem.generated.h"

UENUM(BlueprintType)
enum class EWeatherType : uint8
{
    Clear           UMETA(DisplayName = "Clear"),
    Cloudy          UMETA(DisplayName = "Cloudy"),
    Overcast        UMETA(DisplayName = "Overcast"),
    LightRain       UMETA(DisplayName = "Light Rain"),
    HeavyRain       UMETA(DisplayName = "Heavy Rain"),
    Thunderstorm    UMETA(DisplayName = "Thunderstorm"),
    LightSnow       UMETA(DisplayName = "Light Snow"),
    HeavySnow       UMETA(DisplayName = "Heavy Snow"),
    Blizzard        UMETA(DisplayName = "Blizzard"),
    Fog             UMETA(DisplayName = "Fog"),
    Sandstorm       UMETA(DisplayName = "Sandstorm"),
    Hail            UMETA(DisplayName = "Hail")
};

UENUM(BlueprintType)
enum class ETimeOfDay : uint8
{
    Dawn            UMETA(DisplayName = "Dawn"),
    Morning         UMETA(DisplayName = "Morning"),
    Noon            UMETA(DisplayName = "Noon"),
    Afternoon       UMETA(DisplayName = "Afternoon"),
    Dusk            UMETA(DisplayName = "Dusk"),
    Night           UMETA(DisplayName = "Night"),
    Midnight        UMETA(DisplayName = "Midnight")
};

UENUM(BlueprintType)
enum class ESeason : uint8
{
    Spring          UMETA(DisplayName = "Spring"),
    Summer          UMETA(DisplayName = "Summer"),
    Autumn          UMETA(DisplayName = "Autumn"),
    Winter          UMETA(DisplayName = "Winter")
};

USTRUCT(BlueprintType)
struct FWeatherData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    EWeatherType WeatherType = EWeatherType::Clear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Intensity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float Temperature = 20.0f; // Celsius

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float Humidity = 50.0f; // Percentage

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float Pressure = 1013.25f; // hPa

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    FVector WindDirection = FVector(1, 0, 0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0"))
    float WindSpeed = 5.0f; // m/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float CloudCoverage = 30.0f; // Percentage

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0"))
    float Visibility = 10000.0f; // Meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float PrecipitationRate = 0.0f; // mm/hour

    // Visual effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    class UParticleSystem* PrecipitationEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    class USoundCue* AmbientWeatherSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    FLinearColor SkyTint = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    FLinearColor FogColor = FLinearColor(0.5f, 0.6f, 0.7f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    float FogDensity = 0.02f;
};

USTRUCT(BlueprintType)
struct FTimeOfDayData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float CurrentTime = 12.0f; // 24-hour format

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float TimeSpeed = 1.0f; // Real-time multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    int32 Day = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    int32 Month = 6; // June

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    int32 Year = 2024;

    // Sun/Moon properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    FLinearColor SunColor = FLinearColor(1.0f, 0.95f, 0.8f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    FLinearColor MoonColor = FLinearColor(0.2f, 0.3f, 0.5f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    float SunIntensity = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    float MoonIntensity = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    float SkyLightIntensity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    FRotator SunAngle = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    FRotator MoonAngle = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FEnvironmentalEffects
{
    GENERATED_BODY()

    // Gameplay effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effects")
    float VisibilityModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effects")
    float MovementSpeedModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effects")
    float AccuracyModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effects")
    float SoundAttenuationModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effects")
    float HealthDrainRate = 0.0f; // HP per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effects")
    float StaminaDrainModifier = 1.0f;

    // Equipment effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effects")
    float WeaponDegradationRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effects")
    float ElectronicsReliability = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effects")
    bool bCausesHypothermia = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effects")
    bool bCausesHyperthermia = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effects")
    bool bRequiresSpecialEquipment = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChanged, EWeatherType, OldWeather, EWeatherType, NewWeather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeChanged, float, CurrentTime, ETimeOfDay, TimeOfDay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnvironmentalDamage, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLightningStrike, FVector, StrikeLocation, float, Intensity);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UWeatherEnvironmentalSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeatherEnvironmentalSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    bool bEnableWeatherSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    bool bEnableDynamicWeather = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    bool bEnableTimeProgression = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    bool bEnableSeasonalChanges = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    float WeatherTransitionTime = 300.0f; // 5 minutes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    float EnvironmentalUpdateInterval = 1.0f; // 1 second

    // Current State
    UPROPERTY(BlueprintReadOnly, Category = "Current State")
    FWeatherData CurrentWeather;

    UPROPERTY(BlueprintReadOnly, Category = "Current State")
    FTimeOfDayData TimeData;

    UPROPERTY(BlueprintReadOnly, Category = "Current State")
    ESeason CurrentSeason = ESeason::Summer;

    UPROPERTY(BlueprintReadOnly, Category = "Current State")
    FEnvironmentalEffects ActiveEffects;

    // Weather Presets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Presets")
    TMap<EWeatherType, FWeatherData> WeatherPresets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Presets")
    TArray<EWeatherType> PossibleWeatherTypes;

    // Lighting References
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    class ADirectionalLight* SunLight = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    class ADirectionalLight* MoonLight = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    class ASkyLight* SkyLight = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
    class AExponentialHeightFog* HeightFog = nullptr;

    // Material Parameter Collection for global weather effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    class UMaterialParameterCollection* WeatherMaterialParameters = nullptr;

    // Events
    UPROPERTY(BlueprintAssignable)
    FOnWeatherChanged OnWeatherChanged;

    UPROPERTY(BlueprintAssignable)
    FOnTimeChanged OnTimeChanged;

    UPROPERTY(BlueprintAssignable)
    FOnEnvironmentalDamage OnEnvironmentalDamage;

    UPROPERTY(BlueprintAssignable)
    FOnLightningStrike OnLightningStrike;

    // Weather Control Functions
    UFUNCTION(BlueprintCallable, Category = "Weather Control")
    void SetWeather(EWeatherType NewWeatherType, float TransitionTime = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "Weather Control")
    void SetWeatherIntensity(float NewIntensity);

    UFUNCTION(BlueprintCallable, Category = "Weather Control")
    void TransitionToRandomWeather();

    UFUNCTION(BlueprintCallable, Category = "Weather Control")
    void ForceWeatherUpdate();

    // Time Control Functions
    UFUNCTION(BlueprintCallable, Category = "Time Control")
    void SetTimeOfDay(float NewTime);

    UFUNCTION(BlueprintCallable, Category = "Time Control")
    void SetTimeSpeed(float NewTimeSpeed);

    UFUNCTION(BlueprintCallable, Category = "Time Control")
    void SetDate(int32 NewDay, int32 NewMonth, int32 NewYear);

    UFUNCTION(BlueprintCallable, Category = "Time Control")
    ETimeOfDay GetCurrentTimeOfDay();

    // Environmental Functions
    UFUNCTION(BlueprintCallable, Category = "Environmental")
    FVector GetCurrentWindVector();

    UFUNCTION(BlueprintCallable, Category = "Environmental")
    float GetVisibilityDistance();

    UFUNCTION(BlueprintCallable, Category = "Environmental")
    bool IsRaining();

    UFUNCTION(BlueprintCallable, Category = "Environmental")
    bool IsSnowing();

    UFUNCTION(BlueprintCallable, Category = "Environmental")
    float GetTemperature();

    UFUNCTION(BlueprintCallable, Category = "Environmental")
    float GetHumidity();

    UFUNCTION(BlueprintCallable, Category = "Environmental")
    float GetAirPressure();

    // Effect Functions
    UFUNCTION(BlueprintCallable, Category = "Effects")
    void ApplyEnvironmentalEffects(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Effects")
    void RemoveEnvironmentalEffects(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Effects")
    void TriggerLightningStrike(FVector Location = FVector::ZeroVector);

    UFUNCTION(BlueprintCallable, Category = "Effects")
    void CreateTornado(FVector SpawnLocation, float Intensity = 1.0f);

    // Utility Functions
    UFUNCTION(BlueprintCallable, Category = "Utility")
    float CalculateSunAngle();

    UFUNCTION(BlueprintCallable, Category = "Utility")
    float CalculateMoonAngle();

    UFUNCTION(BlueprintCallable, Category = "Utility")
    ESeason DetermineSeason();

    UFUNCTION(BlueprintCallable, Category = "Utility")
    FLinearColor CalculateSkyColor();

    UFUNCTION(BlueprintCallable, Category = "Utility")
    void InitializeWeatherPresets();

    // Debug Functions
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void LogCurrentWeatherState();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ToggleWeatherDebugDisplay();

private:
    // Internal state
    EWeatherType TargetWeatherType = EWeatherType::Clear;
    float WeatherTransitionProgress = 1.0f;
    float EnvironmentalUpdateTimer = 0.0f;
    float LightningTimer = 0.0f;
    bool bIsTransitioning = false;
    bool bShowDebugInfo = false;

    // Particle system components
    UPROPERTY()
    class UParticleSystemComponent* RainParticleSystem = nullptr;

    UPROPERTY()
    class UParticleSystemComponent* SnowParticleSystem = nullptr;

    UPROPERTY()
    class UParticleSystemComponent* FogParticleSystem = nullptr;

    UPROPERTY()
    class UAudioComponent* WeatherAudioComponent = nullptr;

    // Internal functions
    void UpdateWeatherTransition(float DeltaTime);
    void UpdateTimeProgression(float DeltaTime);
    void UpdateLighting();
    void UpdateParticleEffects();
    void UpdateAudioEffects();
    void UpdateMaterialParameters();
    void UpdateEnvironmentalEffects();
    void ProcessWeatherGameplayEffects(float DeltaTime);

    // Weather transition helpers
    FWeatherData InterpolateWeatherData(const FWeatherData& FromWeather, const FWeatherData& ToWeather, float Alpha);
    void StartWeatherTransition(EWeatherType NewWeatherType);
    void CompleteWeatherTransition();

    // Lighting helpers
    void UpdateSunLighting(float SunAngle);
    void UpdateMoonLighting(float MoonAngle);
    void UpdateSkyLighting();
    void UpdateFogSettings();

    // Random weather generation
    EWeatherType GenerateRandomWeather();
    float GenerateRandomWeatherIntensity(EWeatherType WeatherType);
    bool ShouldTriggerWeatherChange();

    // Environmental calculations
    float CalculateWindEffect(FVector PlayerLocation);
    float CalculateTemperatureEffect();
    float CalculateVisibilityEffect();
    void CalculateSeasonalModifiers();

    // Effect spawning
    void SpawnPrecipitationEffects();
    void SpawnAtmosphericEffects();
    void SpawnLightningEffect(FVector Location);
    void PlayThunderSound(FVector LightningLocation, float Delay);

    // Constants
    static constexpr float EARTH_TILT = 23.45f; // Degrees
    static constexpr float SECONDS_PER_DAY = 86400.0f;
    static constexpr float LIGHTNING_CHANCE_PER_SECOND = 0.1f; // During thunderstorms
};
