#include "AdvancedGraphicsComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "RenderCore.h"
#include "Engine/Engine.h"

UAdvancedGraphicsComponent::UAdvancedGraphicsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize default settings
	GraphicsSettings.QualityLevel = EGraphicsQuality::High;
	GraphicsSettings.bEnableRayTracing = false;
	GraphicsSettings.bEnableDLSS = false;
	GraphicsSettings.bEnableTemporalUpsampling = true;
	GraphicsSettings.bEnableScreenSpaceReflections = true;
	GraphicsSettings.bEnableAmbientOcclusion = true;
	GraphicsSettings.bEnableMotionBlur = true;
	GraphicsSettings.bEnableBloom = true;
	GraphicsSettings.bEnableVolumetricFog = true;
	GraphicsSettings.bEnableLensFlares = true;
	GraphicsSettings.ShadowQuality = 1.0f;
	GraphicsSettings.TextureQuality = 1.0f;
	GraphicsSettings.EffectsQuality = 1.0f;
	GraphicsSettings.ViewDistance = 1.0f;
	GraphicsSettings.AntiAliasingQuality = 1.0f;

	// Initialize weather settings
	WeatherSettings.WeatherType = EWeatherType::Clear;
	WeatherSettings.Intensity = 1.0f;
	WeatherSettings.WindSpeed = 0.5f;
	WeatherSettings.FogColor = FLinearColor::White;
	WeatherSettings.FogDensity = 0.02f;
	WeatherSettings.RainIntensity = 0.0f;
	WeatherSettings.ThunderProbability = 0.0f;

	// Initialize time of day settings
	TimeOfDaySettings.TimeOfDay = ETimeOfDay::Noon;
	TimeOfDaySettings.TimeOfDayProgress = 0.5f;
	TimeOfDaySettings.SunColor = FLinearColor::White;
	TimeOfDaySettings.SunIntensity = 1.0f;
	TimeOfDaySettings.SunRotation = FRotator(-30, 0, 0);
	TimeOfDaySettings.SkyColor = FLinearColor::Blue;
	TimeOfDaySettings.HorizonColor = FLinearColor::White;
	TimeOfDaySettings.bShowStars = false;
	TimeOfDaySettings.MoonIntensity = 0.0f;

	// Performance settings
	CurrentFPS = 60.0f;
	TargetFPS = 60.0f;
	bEnableDynamicQuality = true;
	QualityAdjustmentThreshold = 10.0f;
}

void UAdvancedGraphicsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find scene components
	FindSceneComponents();

	// Apply initial graphics settings
	ApplyGraphicsSettings();

	// Setup dynamic materials
	SetupDynamicMaterials();

	// Initialize weather and time systems
	UpdateLighting();
	CreateWeatherParticles(WeatherSettings.WeatherType);
}

void UAdvancedGraphicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Monitor performance
	MonitorPerformance(DeltaTime);

	// Update weather effects
	UpdateWeather(DeltaTime);

	// Update time of day
	UpdateTimeOfDay(DeltaTime);

	// Update LOD system
	UpdateLOD();

	// Adjust quality based on performance if enabled
	if (bEnableDynamicQuality)
	{
		AdjustQualityForPerformance();
	}
}

void UAdvancedGraphicsComponent::ApplyGraphicsSettings()
{
	UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings();
	if (!UserSettings) return;

	// Apply quality level settings
	switch (GraphicsSettings.QualityLevel)
	{
		case EGraphicsQuality::Low:
			ApplyLowQualityPreset();
			break;
		case EGraphicsQuality::Medium:
			ApplyMediumQualityPreset();
			break;
		case EGraphicsQuality::High:
			ApplyHighQualityPreset();
			break;
		case EGraphicsQuality::Ultra:
			ApplyUltraQualityPreset();
			break;
		case EGraphicsQuality::Cinematic:
			ApplyCinematicQualityPreset();
			break;
	}

	// Apply individual settings
	SetShadowQuality(GraphicsSettings.ShadowQuality);
	SetTextureQuality(GraphicsSettings.TextureQuality);
	SetPostProcessSettings();

	// Apply settings
	UserSettings->ApplySettings(false);
}

void UAdvancedGraphicsComponent::SetGraphicsQuality(EGraphicsQuality Quality)
{
	GraphicsSettings.QualityLevel = Quality;
	ApplyGraphicsSettings();
}

void UAdvancedGraphicsComponent::SetPostProcessSettings()
{
	if (!PostProcessVolume) return;

	FPostProcessSettings& PPSettings = PostProcessVolume->Settings;

	// Motion Blur
	PPSettings.bOverride_MotionBlurAmount = true;
	PPSettings.MotionBlurAmount = GraphicsSettings.bEnableMotionBlur ? 0.5f : 0.0f;

	// Bloom
	PPSettings.bOverride_BloomIntensity = true;
	PPSettings.BloomIntensity = GraphicsSettings.bEnableBloom ? 0.675f : 0.0f;

	// Screen Space Reflections
	PPSettings.bOverride_ScreenSpaceReflectionIntensity = true;
	PPSettings.ScreenSpaceReflectionIntensity = GraphicsSettings.bEnableScreenSpaceReflections ? 100.0f : 0.0f;

	// Ambient Occlusion
	PPSettings.bOverride_AmbientOcclusionIntensity = true;
	PPSettings.AmbientOcclusionIntensity = GraphicsSettings.bEnableAmbientOcclusion ? 0.5f : 0.0f;

	// Volumetric Fog
	PPSettings.bOverride_VolumetricFogDistance = true;
	PPSettings.VolumetricFogDistance = GraphicsSettings.bEnableVolumetricFog ? 6000.0f : 0.0f;
}

void UAdvancedGraphicsComponent::SetShadowQuality(float Quality)
{
	// This would typically be done through console commands or engine settings
	FString Command = FString::Printf(TEXT("r.ShadowQuality %d"), FMath::RoundToInt(Quality * 4));
	if (GEngine)
	{
		GEngine->Exec(GetWorld(), *Command);
	}
}

void UAdvancedGraphicsComponent::SetTextureQuality(float Quality)
{
	FString Command = FString::Printf(TEXT("r.Streaming.PoolSize %d"), FMath::RoundToInt(Quality * 2000));
	if (GEngine)
	{
		GEngine->Exec(GetWorld(), *Command);
	}
}

void UAdvancedGraphicsComponent::UpdateWeather(float DeltaTime)
{
	UpdateWeatherEffects(DeltaTime);

	// Update particle effects if weather is active
	if (ActiveWeatherParticles)
	{
		// Update intensity based on settings
		float TargetRate = 0.0f;
		
		switch (WeatherSettings.WeatherType)
		{
			case EWeatherType::Rain:
			case EWeatherType::Storm:
				TargetRate = WeatherSettings.RainIntensity * 1000.0f;
				break;
			case EWeatherType::Snow:
				TargetRate = WeatherSettings.Intensity * 500.0f;
				break;
			case EWeatherType::Fog:
				// Fog is handled differently
				break;
		}

		// Smoothly adjust particle rate
		// Note: In a real implementation, you'd access the particle system's spawn rate
		// This is a simplified example
	}
}

void UAdvancedGraphicsComponent::SetWeather(EWeatherType Weather, float Intensity)
{
	WeatherSettings.WeatherType = Weather;
	WeatherSettings.Intensity = Intensity;

	CreateWeatherParticles(Weather);
	UpdateLighting();
}

void UAdvancedGraphicsComponent::CreateWeatherParticles(EWeatherType Weather)
{
	// Remove existing weather particles
	if (ActiveWeatherParticles)
	{
		ActiveWeatherParticles->DestroyComponent();
		ActiveWeatherParticles = nullptr;
	}

	UParticleSystem* ParticleSystem = nullptr;

	switch (Weather)
	{
		case EWeatherType::Rain:
		case EWeatherType::Storm:
			ParticleSystem = RainParticleSystem;
			break;
		case EWeatherType::Snow:
			ParticleSystem = SnowParticleSystem;
			break;
		case EWeatherType::Fog:
			ParticleSystem = FogParticleSystem;
			break;
		default:
			return; // No particles for clear weather
	}

	if (ParticleSystem && GetOwner())
	{
		ActiveWeatherParticles = UGameplayStatics::SpawnEmitterAttached(
			ParticleSystem,
			GetOwner()->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepWorldPosition,
			true
		);
	}
}

void UAdvancedGraphicsComponent::UpdateWeatherEffects(float DeltaTime)
{
	// Update fog density for weather
	if (PostProcessVolume)
	{
		FPostProcessSettings& PPSettings = PostProcessVolume->Settings;
		
		float TargetFogDensity = 0.0f;
		FLinearColor TargetFogColor = FLinearColor::White;

		switch (WeatherSettings.WeatherType)
		{
			case EWeatherType::Fog:
				TargetFogDensity = WeatherSettings.FogDensity * WeatherSettings.Intensity;
				TargetFogColor = WeatherSettings.FogColor;
				break;
			case EWeatherType::Storm:
				TargetFogDensity = 0.01f * WeatherSettings.Intensity;
				TargetFogColor = FLinearColor(0.3f, 0.3f, 0.4f, 1.0f);
				break;
		}

		// Smoothly interpolate fog settings
		PPSettings.bOverride_VolumetricFogAlbedo = true;
		PPSettings.VolumetricFogAlbedo = FMath::VInterpTo(
			PPSettings.VolumetricFogAlbedo, 
			TargetFogColor, 
			DeltaTime, 
			2.0f
		);
	}
}

void UAdvancedGraphicsComponent::UpdateTimeOfDay(float DeltaTime)
{
	// This could advance automatically or be controlled externally
	// For now, we just update the lighting based on current settings
	UpdateSunPosition();
	UpdateSkyColors();
	UpdateLighting();
}

void UAdvancedGraphicsComponent::SetTimeOfDay(ETimeOfDay Time)
{
	TimeOfDaySettings.TimeOfDay = Time;

	// Set appropriate progress value
	switch (Time)
	{
		case ETimeOfDay::Dawn:
			TimeOfDaySettings.TimeOfDayProgress = 0.2f;
			break;
		case ETimeOfDay::Morning:
			TimeOfDaySettings.TimeOfDayProgress = 0.3f;
			break;
		case ETimeOfDay::Noon:
			TimeOfDaySettings.TimeOfDayProgress = 0.5f;
			break;
		case ETimeOfDay::Afternoon:
			TimeOfDaySettings.TimeOfDayProgress = 0.7f;
			break;
		case ETimeOfDay::Dusk:
			TimeOfDaySettings.TimeOfDayProgress = 0.8f;
			break;
		case ETimeOfDay::Night:
			TimeOfDaySettings.TimeOfDayProgress = 0.0f;
			break;
	}

	UpdateTimeOfDay(0.0f);
}

void UAdvancedGraphicsComponent::UpdateSunPosition()
{
	if (!SunLight) return;

	// Calculate sun position based on time of day
	float SunAngle = (TimeOfDaySettings.TimeOfDayProgress - 0.5f) * 180.0f; // -90 to +90 degrees
	
	FRotator NewRotation = FRotator(SunAngle, TimeOfDaySettings.SunRotation.Yaw, 0);
	SunLight->GetLightComponent()->SetWorldRotation(NewRotation);

	// Adjust sun intensity based on angle
	float SunHeight = FMath::Sin(FMath::DegreesToRadians(FMath::Abs(SunAngle)));
	float IntensityMultiplier = FMath::Clamp(SunHeight, 0.1f, 1.0f);
	
	SunLight->GetLightComponent()->SetIntensity(TimeOfDaySettings.SunIntensity * IntensityMultiplier);
}

void UAdvancedGraphicsComponent::UpdateSkyColors()
{
	// Update dynamic sky materials if available
	for (UMaterialInstanceDynamic* Material : DynamicMaterials)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("SkyColor"), TimeOfDaySettings.SkyColor);
			Material->SetVectorParameterValue(TEXT("HorizonColor"), TimeOfDaySettings.HorizonColor);
			Material->SetScalarParameterValue(TEXT("TimeOfDay"), TimeOfDaySettings.TimeOfDayProgress);
		}
	}
}

void UAdvancedGraphicsComponent::UpdateLighting()
{
	UpdateSunPosition();
	
	if (SunLight)
	{
		// Adjust sun color based on time of day and weather
		FLinearColor FinalSunColor = TimeOfDaySettings.SunColor;
		
		// Modify color based on weather
		switch (WeatherSettings.WeatherType)
		{
			case EWeatherType::Storm:
				FinalSunColor *= FLinearColor(0.7f, 0.7f, 0.8f, 1.0f);
				break;
			case EWeatherType::Fog:
				FinalSunColor *= FLinearColor(0.9f, 0.9f, 0.95f, 1.0f);
				break;
			case EWeatherType::Overcast:
				FinalSunColor *= FLinearColor(0.8f, 0.8f, 0.85f, 1.0f);
				break;
		}
		
		SunLight->GetLightComponent()->SetLightColor(FinalSunColor);
	}
}

void UAdvancedGraphicsComponent::MonitorPerformance(float DeltaTime)
{
	// Calculate current FPS
	if (DeltaTime > 0.0f)
	{
		float InstantFPS = 1.0f / DeltaTime;
		CurrentFPS = FMath::FInterpTo(CurrentFPS, InstantFPS, DeltaTime, 2.0f);
	}
}

void UAdvancedGraphicsComponent::AdjustQualityForPerformance()
{
	float FPSDifference = TargetFPS - CurrentFPS;
	
	if (FMath::Abs(FPSDifference) > QualityAdjustmentThreshold)
	{
		if (FPSDifference > 0) // FPS too low, reduce quality
		{
			if (GraphicsSettings.QualityLevel > EGraphicsQuality::Low)
			{
				EGraphicsQuality NewQuality = static_cast<EGraphicsQuality>(
					static_cast<int32>(GraphicsSettings.QualityLevel) - 1
				);
				SetGraphicsQuality(NewQuality);
				
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
						TEXT("Graphics quality reduced for performance"));
				}
			}
		}
		else // FPS higher than target, can increase quality
		{
			if (GraphicsSettings.QualityLevel < EGraphicsQuality::Ultra)
			{
				EGraphicsQuality NewQuality = static_cast<EGraphicsQuality>(
					static_cast<int32>(GraphicsSettings.QualityLevel) + 1
				);
				SetGraphicsQuality(NewQuality);
				
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
						TEXT("Graphics quality increased"));
				}
			}
		}
	}
}

void UAdvancedGraphicsComponent::UpdateLOD()
{
	// This would typically manage LOD levels for meshes based on distance
	// For now, we'll just implement a basic example
	
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player || !GetOwner()) return;

	float DistanceToPlayer = FVector::Dist(GetOwner()->GetActorLocation(), Player->GetActorLocation());
	
	// Determine LOD level based on distance
	int32 LODLevel = 0;
	if (DistanceToPlayer > LODDistance3)
		LODLevel = 3;
	else if (DistanceToPlayer > LODDistance2)
		LODLevel = 2;
	else if (DistanceToPlayer > LODDistance1)
		LODLevel = 1;

	// Apply LOD to skeletal meshes
	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	GetOwner()->GetComponents<USkeletalMeshComponent>(SkeletalMeshes);
	
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (SkeletalMesh)
		{
			SkeletalMesh->SetForcedLOD(LODLevel + 1); // UE uses 1-based LOD indices
		}
	}
}

void UAdvancedGraphicsComponent::FindSceneComponents()
{
	// Find post process volume
	TArray<AActor*> PostProcessVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APostProcessVolume::StaticClass(), PostProcessVolumes);
	
	if (PostProcessVolumes.Num() > 0)
	{
		PostProcessVolume = Cast<APostProcessVolume>(PostProcessVolumes[0]);
	}

	// Find directional light (sun)
	TArray<AActor*> DirectionalLights;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADirectionalLight::StaticClass(), DirectionalLights);
	
	if (DirectionalLights.Num() > 0)
	{
		SunLight = Cast<ADirectionalLight>(DirectionalLights[0]);
	}

	// Find sky sphere (if available)
	// This would depend on your specific sky sphere implementation
}

void UAdvancedGraphicsComponent::SetupDynamicMaterials()
{
	// Setup dynamic materials for sky and other effects
	// This would be implemented based on your specific material setup
}

// Public interface implementations

void UAdvancedGraphicsComponent::SetGraphicsSettings(FGraphicsSettings NewSettings)
{
	GraphicsSettings = NewSettings;
	ApplyGraphicsSettings();
}

void UAdvancedGraphicsComponent::SetWeatherSettings(FWeatherSettings NewSettings)
{
	WeatherSettings = NewSettings;
	CreateWeatherParticles(NewSettings.WeatherType);
	UpdateLighting();
}

void UAdvancedGraphicsComponent::TransitionToWeather(EWeatherType NewWeather, float TransitionTime)
{
	// Implement weather transition over time
	WeatherSettings.WeatherType = NewWeather;
	CreateWeatherParticles(NewWeather);
	UpdateLighting();
}

void UAdvancedGraphicsComponent::SetTimeOfDaySettings(FTimeOfDaySettings NewSettings)
{
	TimeOfDaySettings = NewSettings;
	UpdateTimeOfDay(0.0f);
}

void UAdvancedGraphicsComponent::SetTimeOfDayProgress(float Progress)
{
	TimeOfDaySettings.TimeOfDayProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	UpdateTimeOfDay(0.0f);
}

void UAdvancedGraphicsComponent::AdvanceTimeOfDay(float Hours)
{
	float DayProgress = Hours / 24.0f;
	TimeOfDaySettings.TimeOfDayProgress += DayProgress;
	
	if (TimeOfDaySettings.TimeOfDayProgress > 1.0f)
	{
		TimeOfDaySettings.TimeOfDayProgress -= 1.0f;
	}
	
	UpdateTimeOfDay(0.0f);
}

void UAdvancedGraphicsComponent::AddDynamicMaterial(UMaterialInterface* Material, UPrimitiveComponent* Component)
{
	if (Material && Component)
	{
		UMaterialInstanceDynamic* DynamicMaterial = Component->CreateDynamicMaterialInstance(0, Material);
		if (DynamicMaterial)
		{
			DynamicMaterials.Add(DynamicMaterial);
		}
	}
}

void UAdvancedGraphicsComponent::UpdateMaterialParameter(const FString& ParameterName, float Value)
{
	for (UMaterialInstanceDynamic* Material : DynamicMaterials)
	{
		if (Material)
		{
			Material->SetScalarParameterValue(FName(*ParameterName), Value);
		}
	}
}

void UAdvancedGraphicsComponent::UpdateMaterialVectorParameter(const FString& ParameterName, FLinearColor Value)
{
	for (UMaterialInstanceDynamic* Material : DynamicMaterials)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(FName(*ParameterName), Value);
		}
	}
}

// Quality preset implementations

void UAdvancedGraphicsComponent::ApplyLowQualityPreset()
{
	GraphicsSettings.bEnableRayTracing = false;
	GraphicsSettings.bEnableDLSS = false;
	GraphicsSettings.bEnableScreenSpaceReflections = false;
	GraphicsSettings.bEnableAmbientOcclusion = false;
	GraphicsSettings.bEnableMotionBlur = false;
	GraphicsSettings.bEnableVolumetricFog = false;
	GraphicsSettings.ShadowQuality = 0.3f;
	GraphicsSettings.TextureQuality = 0.5f;
	GraphicsSettings.EffectsQuality = 0.5f;
	GraphicsSettings.ViewDistance = 0.6f;
	GraphicsSettings.AntiAliasingQuality = 0.3f;
}

void UAdvancedGraphicsComponent::ApplyMediumQualityPreset()
{
	GraphicsSettings.bEnableRayTracing = false;
	GraphicsSettings.bEnableDLSS = true;
	GraphicsSettings.bEnableScreenSpaceReflections = false;
	GraphicsSettings.bEnableAmbientOcclusion = true;
	GraphicsSettings.bEnableMotionBlur = true;
	GraphicsSettings.bEnableVolumetricFog = false;
	GraphicsSettings.ShadowQuality = 0.6f;
	GraphicsSettings.TextureQuality = 0.7f;
	GraphicsSettings.EffectsQuality = 0.7f;
	GraphicsSettings.ViewDistance = 0.8f;
	GraphicsSettings.AntiAliasingQuality = 0.6f;
}

void UAdvancedGraphicsComponent::ApplyHighQualityPreset()
{
	GraphicsSettings.bEnableRayTracing = false;
	GraphicsSettings.bEnableDLSS = true;
	GraphicsSettings.bEnableScreenSpaceReflections = true;
	GraphicsSettings.bEnableAmbientOcclusion = true;
	GraphicsSettings.bEnableMotionBlur = true;
	GraphicsSettings.bEnableVolumetricFog = true;
	GraphicsSettings.ShadowQuality = 0.8f;
	GraphicsSettings.TextureQuality = 0.9f;
	GraphicsSettings.EffectsQuality = 0.9f;
	GraphicsSettings.ViewDistance = 1.0f;
	GraphicsSettings.AntiAliasingQuality = 0.8f;
}

void UAdvancedGraphicsComponent::ApplyUltraQualityPreset()
{
	GraphicsSettings.bEnableRayTracing = true;
	GraphicsSettings.bEnableDLSS = true;
	GraphicsSettings.bEnableScreenSpaceReflections = true;
	GraphicsSettings.bEnableAmbientOcclusion = true;
	GraphicsSettings.bEnableMotionBlur = true;
	GraphicsSettings.bEnableVolumetricFog = true;
	GraphicsSettings.ShadowQuality = 1.0f;
	GraphicsSettings.TextureQuality = 1.0f;
	GraphicsSettings.EffectsQuality = 1.0f;
	GraphicsSettings.ViewDistance = 1.0f;
	GraphicsSettings.AntiAliasingQuality = 1.0f;
}

void UAdvancedGraphicsComponent::ApplyCinematicQualityPreset()
{
	GraphicsSettings.bEnableRayTracing = true;
	GraphicsSettings.bEnableDLSS = false; // Use native resolution for cinematic
	GraphicsSettings.bEnableScreenSpaceReflections = true;
	GraphicsSettings.bEnableAmbientOcclusion = true;
	GraphicsSettings.bEnableMotionBlur = true;
	GraphicsSettings.bEnableVolumetricFog = true;
	GraphicsSettings.bEnableLensFlares = true;
	GraphicsSettings.ShadowQuality = 1.2f;
	GraphicsSettings.TextureQuality = 1.2f;
	GraphicsSettings.EffectsQuality = 1.2f;
	GraphicsSettings.ViewDistance = 1.2f;
	GraphicsSettings.AntiAliasingQuality = 1.2f;
}

void UAdvancedGraphicsComponent::ToggleDebugMode()
{
	// Toggle debug information display
	if (GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("stat fps"));
	}
}

void UAdvancedGraphicsComponent::ShowPerformanceStats(bool bShow)
{
	if (GEngine)
	{
		if (bShow)
		{
			GEngine->Exec(GetWorld(), TEXT("stat fps"));
			GEngine->Exec(GetWorld(), TEXT("stat unit"));
		}
		else
		{
			GEngine->Exec(GetWorld(), TEXT("stat none"));
		}
	}
}
