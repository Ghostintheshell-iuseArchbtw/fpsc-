#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Engine/PostProcessVolume.h"
#include "Components/PostProcessComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "AdvancedGraphicsComponent.generated.h"

UENUM(BlueprintType)
enum class EGraphicsQuality : uint8
{
	Low,
	Medium,
	High,
	Ultra,
	Cinematic
};

UENUM(BlueprintType)
enum class EWeatherType : uint8
{
	Clear,
	Overcast,
	Rain,
	Storm,
	Fog,
	Snow
};

UENUM(BlueprintType)
enum class ETimeOfDay : uint8
{
	Dawn,
	Morning,
	Noon,
	Afternoon,
	Dusk,
	Night
};

USTRUCT(BlueprintType)
struct FGraphicsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGraphicsQuality QualityLevel = EGraphicsQuality::High;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableRayTracing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableDLSS = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableTemporalUpsampling = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableScreenSpaceReflections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableAmbientOcclusion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableMotionBlur = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableBloom = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableVolumetricFog = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableLensFlares = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ShadowQuality = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TextureQuality = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectsQuality = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ViewDistance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AntiAliasingQuality = 1.0f;
};

USTRUCT(BlueprintType)
struct FWeatherSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeatherType WeatherType = EWeatherType::Clear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WindSpeed = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor FogColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FogDensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RainIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThunderProbability = 0.0f;
};

USTRUCT(BlueprintType)
struct FTimeOfDaySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETimeOfDay TimeOfDay = ETimeOfDay::Noon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeOfDayProgress = 0.5f; // 0-1 through the day

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor SunColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SunIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator SunRotation = FRotator(-30, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor SkyColor = FLinearColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor HorizonColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowStars = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoonIntensity = 0.0f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UAdvancedGraphicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAdvancedGraphicsComponent();

protected:
	virtual void BeginPlay() override;

	// Graphics settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphics")
	FGraphicsSettings GraphicsSettings;

	// Weather system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FWeatherSettings WeatherSettings;

	// Time of day system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	FTimeOfDaySettings TimeOfDaySettings;

	// Post process volume reference
	UPROPERTY(BlueprintReadOnly, Category = "Post Process")
	APostProcessVolume* PostProcessVolume;

	// Directional light reference (sun)
	UPROPERTY(BlueprintReadOnly, Category = "Lighting")
	ADirectionalLight* SunLight;

	// Sky sphere reference
	UPROPERTY(BlueprintReadOnly, Category = "Sky")
	AActor* SkySphere;

	// Particle systems for weather
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	UParticleSystem* RainParticleSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	UParticleSystem* SnowParticleSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	UParticleSystem* FogParticleSystem;

	// Active weather particle component
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	UParticleSystemComponent* ActiveWeatherParticles;

	// Material instances for dynamic effects
	UPROPERTY(BlueprintReadOnly, Category = "Materials")
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	// LOD management
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float LODDistance1 = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float LODDistance2 = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float LODDistance3 = 2000.0f;

	// Performance monitoring
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float CurrentFPS = 60.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float TargetFPS = 60.0f;

	// Dynamic quality adjustment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	bool bEnableDynamicQuality = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float QualityAdjustmentThreshold = 10.0f;

	// Graphics functions
	void ApplyGraphicsSettings();
	void SetGraphicsQuality(EGraphicsQuality Quality);
	void SetPostProcessSettings();
	void SetShadowQuality(float Quality);
	void SetTextureQuality(float Quality);

	// Weather functions
	void UpdateWeather(float DeltaTime);
	void SetWeather(EWeatherType Weather, float Intensity);
	void CreateWeatherParticles(EWeatherType Weather);
	void UpdateWeatherEffects(float DeltaTime);

	// Time of day functions
	void UpdateTimeOfDay(float DeltaTime);
	void SetTimeOfDay(ETimeOfDay Time);
	void UpdateSunPosition();
	void UpdateSkyColors();
	void UpdateLighting();

	// Performance functions
	void MonitorPerformance(float DeltaTime);
	void AdjustQualityForPerformance();
	void UpdateLOD();

	// Utility functions
	void FindSceneComponents();
	void SetupDynamicMaterials();

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Public interface for graphics settings
	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void SetGraphicsSettings(FGraphicsSettings NewSettings);

	UFUNCTION(BlueprintPure, Category = "Graphics")
	FGraphicsSettings GetGraphicsSettings() const { return GraphicsSettings; }

	// Weather control
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeatherSettings(FWeatherSettings NewSettings);

	UFUNCTION(BlueprintPure, Category = "Weather")
	FWeatherSettings GetWeatherSettings() const { return WeatherSettings; }

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void TransitionToWeather(EWeatherType NewWeather, float TransitionTime = 5.0f);

	// Time of day control
	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetTimeOfDaySettings(FTimeOfDaySettings NewSettings);

	UFUNCTION(BlueprintPure, Category = "Time")
	FTimeOfDaySettings GetTimeOfDaySettings() const { return TimeOfDaySettings; }

	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetTimeOfDayProgress(float Progress); // 0-1 through the day

	UFUNCTION(BlueprintCallable, Category = "Time")
	void AdvanceTimeOfDay(float Hours);

	// Performance monitoring
	UFUNCTION(BlueprintPure, Category = "Performance")
	float GetCurrentFPS() const { return CurrentFPS; }

	UFUNCTION(BlueprintPure, Category = "Performance")
	float GetFrameTime() const { return 1.0f / CurrentFPS; }

	UFUNCTION(BlueprintCallable, Category = "Performance")
	void SetTargetFPS(float NewTargetFPS) { TargetFPS = NewTargetFPS; }

	// Material effects
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void AddDynamicMaterial(UMaterialInterface* Material, UPrimitiveComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "Effects")
	void UpdateMaterialParameter(const FString& ParameterName, float Value);

	UFUNCTION(BlueprintCallable, Category = "Effects")
	void UpdateMaterialVectorParameter(const FString& ParameterName, FLinearColor Value);

	// Quality presets
	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void ApplyLowQualityPreset();

	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void ApplyMediumQualityPreset();

	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void ApplyHighQualityPreset();

	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void ApplyUltraQualityPreset();

	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void ApplyCinematicQualityPreset();

	// Debug functions
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ToggleDebugMode();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ShowPerformanceStats(bool bShow);
};
