#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundAttenuation.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "AdvancedAudioSystem.generated.h"

UENUM(BlueprintType)
enum class EAudioLayer : uint8
{
	Master,
	Music,
	SFX,
	Voice,
	Ambient,
	UI,
	Weapon
};

UENUM(BlueprintType)
enum class EAudioEnvironment : uint8
{
	None,
	SmallRoom,
	LargeRoom,
	Hall,
	Cave,
	Arena,
	Hangar,
	Carpeted,
	Bathroom,
	Underwater,
	Mountains,
	Forest,
	City,
	Tunnel
};

UENUM(BlueprintType)
enum class EMusicState : uint8
{
	Calm,
	Tension,
	Combat,
	Victory,
	Defeat,
	Menu,
	Ambient
};

USTRUCT(BlueprintType)
struct FAudioSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MusicVolume = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VoiceVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AmbientVolume = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UIVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnable3DAudio = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableReverb = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableOcclusion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableDopplerEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ReverbMix = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OcclusionThreshold = 100.0f;
};

USTRUCT(BlueprintType)
struct FAudioCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* SoundCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAudioLayer AudioLayer = EAudioLayer::SFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Pitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIs3D = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoDestroy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FadeInTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FadeOutTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FMusicTrack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* MusicCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMusicState MusicState = EMusicState::Calm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CrossfadeTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLoop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Priority = 1.0f;
};

USTRUCT(BlueprintType)
struct FAmbientSoundInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SoundName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAudioComponent* AudioComponent = nullptr;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSGAME_API UAdvancedAudioSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAdvancedAudioSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Audio settings
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetAudioSettings(const FAudioSettings& NewSettings);

	UFUNCTION(BlueprintPure, Category = "Audio")
	FAudioSettings GetAudioSettings() const { return AudioSettings; }

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetLayerVolume(EAudioLayer Layer, float Volume);

	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetLayerVolume(EAudioLayer Layer) const;

	// 3D Audio playback
	UFUNCTION(BlueprintCallable, Category = "Audio")
	UAudioComponent* PlaySoundAtLocation(const FAudioCue& AudioCue, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	UAudioComponent* PlaySoundAttached(const FAudioCue& AudioCue, USceneComponent* AttachToComponent, const FVector& LocationOffset = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	UAudioComponent* PlaySound2D(const FAudioCue& AudioCue);

	// Music system
	UFUNCTION(BlueprintCallable, Category = "Music")
	void PlayMusic(const FMusicTrack& MusicTrack);

	UFUNCTION(BlueprintCallable, Category = "Music")
	void SetMusicState(EMusicState NewState);

	UFUNCTION(BlueprintCallable, Category = "Music")
	void CrossfadeToMusic(const FMusicTrack& NewMusicTrack, float CrossfadeTime = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Music")
	void StopMusic(float FadeOutTime = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Music")
	void PauseMusic();

	UFUNCTION(BlueprintCallable, Category = "Music")
	void ResumeMusic();

	// Ambient sound management
	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void AddAmbientSound(const FString& SoundName, USoundCue* SoundCue, const FVector& Location, float Volume = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void RemoveAmbientSound(const FString& SoundName);

	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void SetAmbientSoundVolume(const FString& SoundName, float Volume);

	UFUNCTION(BlueprintCallable, Category = "Ambient")
	void SetAmbientSoundLocation(const FString& SoundName, const FVector& NewLocation);

	// Environmental audio
	UFUNCTION(BlueprintCallable, Category = "Environment")
	void SetAudioEnvironment(EAudioEnvironment Environment);

	UFUNCTION(BlueprintPure, Category = "Environment")
	EAudioEnvironment GetCurrentAudioEnvironment() const { return CurrentEnvironment; }

	// Weapon audio
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void PlayWeaponSound(USoundCue* WeaponSound, const FVector& Location, float VolumeMultiplier = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void PlayWeaponSoundWithEcho(USoundCue* WeaponSound, const FVector& Location, float EchoDelay = 0.3f, float EchoVolume = 0.5f);

	// Utility functions
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopAllSounds();

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopSoundsByLayer(EAudioLayer Layer);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void FadeAllSounds(float FadeTime, float TargetVolume = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	float CalculateDistanceAttenuation(const FVector& SoundLocation, const FVector& ListenerLocation, float MinDistance, float MaxDistance) const;

	UFUNCTION(BlueprintCallable, Category = "Audio")
	bool IsLocationOccluded(const FVector& SoundLocation, const FVector& ListenerLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void UpdateListenerLocation(const FVector& Location, const FRotator& Rotation);

	// Voice chat
	UFUNCTION(BlueprintCallable, Category = "Voice")
	void SetVoiceChatEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void SetVoiceChatVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void MutePlayer(const FString& PlayerName);

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void UnmutePlayer(const FString& PlayerName);

protected:
	// Audio settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FAudioSettings AudioSettings;

	// Current environment
	UPROPERTY(BlueprintReadOnly, Category = "Environment")
	EAudioEnvironment CurrentEnvironment = EAudioEnvironment::None;

	// Current music state
	UPROPERTY(BlueprintReadOnly, Category = "Music")
	EMusicState CurrentMusicState = EMusicState::Calm;

	// Music tracks for different states
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	TMap<EMusicState, FMusicTrack> MusicTracks;

	// Current music component
	UPROPERTY(BlueprintReadOnly, Category = "Music")
	UAudioComponent* CurrentMusicComponent = nullptr;

	// Previous music component (for crossfading)
	UPROPERTY(BlueprintReadOnly, Category = "Music")
	UAudioComponent* PreviousMusicComponent = nullptr;

	// Ambient sounds
	UPROPERTY(BlueprintReadOnly, Category = "Ambient")
	TMap<FString, FAmbientSoundInfo> AmbientSounds;

	// Active audio components by layer
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	TMap<EAudioLayer, TArray<UAudioComponent*>> ActiveAudioComponents;

	// Voice chat settings
	UPROPERTY(BlueprintReadOnly, Category = "Voice")
	bool bVoiceChatEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Voice")
	float VoiceChatVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Voice")
	TSet<FString> MutedPlayers;

	// Audio processing
	void ProcessOcclusion(UAudioComponent* AudioComponent, const FVector& SoundLocation);
	void ProcessDopplerEffect(UAudioComponent* AudioComponent, const FVector& SoundLocation, const FVector& SoundVelocity);
	void ProcessEnvironmentalEffects(UAudioComponent* AudioComponent);
	void UpdateMusicCrossfade(float DeltaTime);
	void CleanupFinishedAudioComponents();

	// Helper functions
	float GetVolumeForLayer(EAudioLayer Layer) const;
	USoundAttenuation* GetAttenuationSettingsForEnvironment(EAudioEnvironment Environment);
	void ApplyLayerVolumeToComponent(UAudioComponent* AudioComponent, EAudioLayer Layer);

	// Listener tracking
	FVector LastListenerLocation = FVector::ZeroVector;
	FRotator LastListenerRotation = FRotator::ZeroRotator;
	FVector ListenerVelocity = FVector::ZeroVector;

	// Music crossfade
	float CrossfadeTimer = 0.0f;
	float CrossfadeDuration = 0.0f;
	bool bIsCrossfading = false;

	// Performance optimization
	float AudioUpdateInterval = 0.1f; // Update every 100ms
	float TimeSinceLastUpdate = 0.0f;
};
