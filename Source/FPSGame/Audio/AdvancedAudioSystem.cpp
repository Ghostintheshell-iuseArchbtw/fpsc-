#include "AdvancedAudioSystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"

UAdvancedAudioSystem::UAdvancedAudioSystem()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize default audio settings
	AudioSettings.MasterVolume = 1.0f;
	AudioSettings.MusicVolume = 0.7f;
	AudioSettings.SFXVolume = 1.0f;
	AudioSettings.VoiceVolume = 1.0f;
	AudioSettings.AmbientVolume = 0.6f;
	AudioSettings.UIVolume = 0.8f;
	AudioSettings.WeaponVolume = 1.0f;
	AudioSettings.bEnable3DAudio = true;
	AudioSettings.bEnableReverb = true;
	AudioSettings.bEnableOcclusion = true;
	AudioSettings.bEnableDopplerEffect = true;
	AudioSettings.ReverbMix = 0.5f;
	AudioSettings.OcclusionThreshold = 100.0f;

	// Initialize music tracks (would be set in Blueprint or loaded from data)
	// This is just example setup
	CurrentMusicState = EMusicState::Menu;
	CurrentEnvironment = EAudioEnvironment::None;
}

void UAdvancedAudioSystem::BeginPlay()
{
	Super::BeginPlay();

	// Setup initial audio environment
	SetAudioEnvironment(EAudioEnvironment::None);

	// Initialize active audio components arrays
	for (int32 i = 0; i < (int32)EAudioLayer::Weapon + 1; i++)
	{
		ActiveAudioComponents.Add((EAudioLayer)i, TArray<UAudioComponent*>());
	}
}

void UAdvancedAudioSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastUpdate += DeltaTime;

	// Update at reduced frequency for performance
	if (TimeSinceLastUpdate >= AudioUpdateInterval)
	{
		TimeSinceLastUpdate = 0.0f;

		// Update listener position
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			FVector NewLocation;
			FRotator NewRotation;
			PC->GetPlayerViewPoint(NewLocation, NewRotation);
			
			// Calculate velocity
			ListenerVelocity = (NewLocation - LastListenerLocation) / AudioUpdateInterval;
			
			UpdateListenerLocation(NewLocation, NewRotation);
		}

		// Update music crossfade
		if (bIsCrossfading)
		{
			UpdateMusicCrossfade(DeltaTime);
		}

		// Process 3D audio effects
		if (AudioSettings.bEnable3DAudio)
		{
			for (auto& LayerPair : ActiveAudioComponents)
			{
				for (UAudioComponent* AudioComp : LayerPair.Value)
				{
					if (AudioComp && IsValid(AudioComp))
					{
						FVector SoundLocation = AudioComp->GetComponentLocation();
						
						// Process occlusion
						if (AudioSettings.bEnableOcclusion)
						{
							ProcessOcclusion(AudioComp, SoundLocation);
						}

						// Process Doppler effect
						if (AudioSettings.bEnableDopplerEffect)
						{
							// For simplicity, assume zero velocity for sound source
							ProcessDopplerEffect(AudioComp, SoundLocation, FVector::ZeroVector);
						}

						// Process environmental effects
						if (AudioSettings.bEnableReverb)
						{
							ProcessEnvironmentalEffects(AudioComp);
						}
					}
				}
			}
		}

		// Clean up finished audio components
		CleanupFinishedAudioComponents();
	}
}

void UAdvancedAudioSystem::SetAudioSettings(const FAudioSettings& NewSettings)
{
	AudioSettings = NewSettings;

	// Apply volume changes to all active components
	for (auto& LayerPair : ActiveAudioComponents)
	{
		EAudioLayer Layer = LayerPair.Key;
		for (UAudioComponent* AudioComp : LayerPair.Value)
		{
			if (AudioComp && IsValid(AudioComp))
			{
				ApplyLayerVolumeToComponent(AudioComp, Layer);
			}
		}
	}

	// Update music volume
	if (CurrentMusicComponent && IsValid(CurrentMusicComponent))
	{
		CurrentMusicComponent->SetVolumeMultiplier(AudioSettings.MasterVolume * AudioSettings.MusicVolume);
	}
}

void UAdvancedAudioSystem::SetLayerVolume(EAudioLayer Layer, float Volume)
{
	switch (Layer)
	{
		case EAudioLayer::Master:
			AudioSettings.MasterVolume = Volume;
			break;
		case EAudioLayer::Music:
			AudioSettings.MusicVolume = Volume;
			break;
		case EAudioLayer::SFX:
			AudioSettings.SFXVolume = Volume;
			break;
		case EAudioLayer::Voice:
			AudioSettings.VoiceVolume = Volume;
			break;
		case EAudioLayer::Ambient:
			AudioSettings.AmbientVolume = Volume;
			break;
		case EAudioLayer::UI:
			AudioSettings.UIVolume = Volume;
			break;
		case EAudioLayer::Weapon:
			AudioSettings.WeaponVolume = Volume;
			break;
	}

	// Apply to active components of this layer
	if (ActiveAudioComponents.Contains(Layer))
	{
		for (UAudioComponent* AudioComp : ActiveAudioComponents[Layer])
		{
			if (AudioComp && IsValid(AudioComp))
			{
				ApplyLayerVolumeToComponent(AudioComp, Layer);
			}
		}
	}
}

float UAdvancedAudioSystem::GetLayerVolume(EAudioLayer Layer) const
{
	return GetVolumeForLayer(Layer);
}

UAudioComponent* UAdvancedAudioSystem::PlaySoundAtLocation(const FAudioCue& AudioCue, const FVector& Location, const FRotator& Rotation)
{
	if (!AudioCue.SoundCue)
	{
		return nullptr;
	}

	UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		GetWorld(),
		AudioCue.SoundCue,
		Location,
		Rotation,
		AudioCue.Volume * GetVolumeForLayer(AudioCue.AudioLayer),
		AudioCue.Pitch,
		AudioCue.FadeInTime,
		nullptr, // Attenuation will be handled manually
		nullptr,
		AudioCue.bAutoDestroy
	);

	if (AudioComponent)
	{
		// Configure 3D audio properties
		if (AudioCue.bIs3D && AudioSettings.bEnable3DAudio)
		{
			AudioComponent->bAllowSpatialization = true;
			AudioComponent->AttenuationOverrides.bAttenuate = true;
			AudioComponent->AttenuationOverrides.bSpatialize = true;
			AudioComponent->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
			AudioComponent->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
			AudioComponent->AttenuationOverrides.FalloffDistance = AudioCue.MaxDistance - AudioCue.MinDistance;
			AudioComponent->AttenuationOverrides.AttenuationShapeExtents = FVector(AudioCue.MaxDistance);
		}
		else
		{
			AudioComponent->bAllowSpatialization = false;
			AudioComponent->AttenuationOverrides.bAttenuate = false;
		}

		// Add to active components
		if (ActiveAudioComponents.Contains(AudioCue.AudioLayer))
		{
			ActiveAudioComponents[AudioCue.AudioLayer].Add(AudioComponent);
		}

		// Apply layer volume
		ApplyLayerVolumeToComponent(AudioComponent, AudioCue.AudioLayer);
	}

	return AudioComponent;
}

UAudioComponent* UAdvancedAudioSystem::PlaySoundAttached(const FAudioCue& AudioCue, USceneComponent* AttachToComponent, const FVector& LocationOffset)
{
	if (!AudioCue.SoundCue || !AttachToComponent)
	{
		return nullptr;
	}

	UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAttached(
		AudioCue.SoundCue,
		AttachToComponent,
		NAME_None,
		LocationOffset,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true,
		AudioCue.Volume * GetVolumeForLayer(AudioCue.AudioLayer),
		AudioCue.Pitch,
		AudioCue.FadeInTime,
		nullptr,
		nullptr,
		AudioCue.bAutoDestroy
	);

	if (AudioComponent)
	{
		// Configure 3D audio properties (same as PlaySoundAtLocation)
		if (AudioCue.bIs3D && AudioSettings.bEnable3DAudio)
		{
			AudioComponent->bAllowSpatialization = true;
			AudioComponent->AttenuationOverrides.bAttenuate = true;
			AudioComponent->AttenuationOverrides.bSpatialize = true;
			AudioComponent->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
			AudioComponent->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
			AudioComponent->AttenuationOverrides.FalloffDistance = AudioCue.MaxDistance - AudioCue.MinDistance;
			AudioComponent->AttenuationOverrides.AttenuationShapeExtents = FVector(AudioCue.MaxDistance);
		}

		// Add to active components
		if (ActiveAudioComponents.Contains(AudioCue.AudioLayer))
		{
			ActiveAudioComponents[AudioCue.AudioLayer].Add(AudioComponent);
		}

		ApplyLayerVolumeToComponent(AudioComponent, AudioCue.AudioLayer);
	}

	return AudioComponent;
}

UAudioComponent* UAdvancedAudioSystem::PlaySound2D(const FAudioCue& AudioCue)
{
	if (!AudioCue.SoundCue)
	{
		return nullptr;
	}

	UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(
		GetWorld(),
		AudioCue.SoundCue,
		AudioCue.Volume * GetVolumeForLayer(AudioCue.AudioLayer),
		AudioCue.Pitch,
		AudioCue.FadeInTime,
		nullptr,
		AudioCue.bAutoDestroy
	);

	if (AudioComponent)
	{
		// Disable spatialization for 2D sound
		AudioComponent->bAllowSpatialization = false;
		AudioComponent->AttenuationOverrides.bAttenuate = false;

		// Add to active components
		if (ActiveAudioComponents.Contains(AudioCue.AudioLayer))
		{
			ActiveAudioComponents[AudioCue.AudioLayer].Add(AudioComponent);
		}

		ApplyLayerVolumeToComponent(AudioComponent, AudioCue.AudioLayer);
	}

	return AudioComponent;
}

void UAdvancedAudioSystem::PlayMusic(const FMusicTrack& MusicTrack)
{
	if (!MusicTrack.MusicCue)
	{
		return;
	}

	// Stop current music
	if (CurrentMusicComponent && IsValid(CurrentMusicComponent))
	{
		CurrentMusicComponent->FadeOut(1.0f, 0.0f);
	}

	// Play new music
	CurrentMusicComponent = UGameplayStatics::SpawnSound2D(
		GetWorld(),
		MusicTrack.MusicCue,
		MusicTrack.Volume * AudioSettings.MasterVolume * AudioSettings.MusicVolume,
		1.0f,
		0.0f,
		nullptr,
		false // Don't auto-destroy
	);

	if (CurrentMusicComponent)
	{
		CurrentMusicComponent->bAllowSpatialization = false;
		CurrentMusicComponent->SetUISound(true);
		
		if (MusicTrack.bLoop)
		{
			CurrentMusicComponent->SetIntParameter(FName("bLoop"), 1);
		}
	}

	CurrentMusicState = MusicTrack.MusicState;
}

void UAdvancedAudioSystem::SetMusicState(EMusicState NewState)
{
	if (CurrentMusicState == NewState)
	{
		return;
	}

	CurrentMusicState = NewState;

	// Find music track for this state
	if (MusicTracks.Contains(NewState))
	{
		const FMusicTrack& Track = MusicTracks[NewState];
		CrossfadeToMusic(Track, Track.CrossfadeTime);
	}
}

void UAdvancedAudioSystem::CrossfadeToMusic(const FMusicTrack& NewMusicTrack, float CrossfadeTime)
{
	if (!NewMusicTrack.MusicCue)
	{
		return;
	}

	// Store previous music component for fade out
	PreviousMusicComponent = CurrentMusicComponent;

	// Start new music
	CurrentMusicComponent = UGameplayStatics::SpawnSound2D(
		GetWorld(),
		NewMusicTrack.MusicCue,
		0.0f, // Start at 0 volume for crossfade
		1.0f,
		0.0f,
		nullptr,
		false
	);

	if (CurrentMusicComponent)
	{
		CurrentMusicComponent->bAllowSpatialization = false;
		CurrentMusicComponent->SetUISound(true);
		
		if (NewMusicTrack.bLoop)
		{
			CurrentMusicComponent->SetIntParameter(FName("bLoop"), 1);
		}

		// Setup crossfade
		CrossfadeTimer = 0.0f;
		CrossfadeDuration = CrossfadeTime;
		bIsCrossfading = true;
	}

	CurrentMusicState = NewMusicTrack.MusicState;
}

void UAdvancedAudioSystem::StopMusic(float FadeOutTime)
{
	if (CurrentMusicComponent && IsValid(CurrentMusicComponent))
	{
		if (FadeOutTime > 0.0f)
		{
			CurrentMusicComponent->FadeOut(FadeOutTime, 0.0f);
		}
		else
		{
			CurrentMusicComponent->Stop();
		}
	}
	CurrentMusicComponent = nullptr;
}

void UAdvancedAudioSystem::PauseMusic()
{
	if (CurrentMusicComponent && IsValid(CurrentMusicComponent))
	{
		CurrentMusicComponent->SetPaused(true);
	}
}

void UAdvancedAudioSystem::ResumeMusic()
{
	if (CurrentMusicComponent && IsValid(CurrentMusicComponent))
	{
		CurrentMusicComponent->SetPaused(false);
	}
}

void UAdvancedAudioSystem::AddAmbientSound(const FString& SoundName, USoundCue* SoundCue, const FVector& Location, float Volume)
{
	if (!SoundCue)
	{
		return;
	}

	// Remove existing ambient sound with same name
	RemoveAmbientSound(SoundName);

	// Create new ambient sound
	FAmbientSoundInfo AmbientInfo;
	AmbientInfo.SoundName = SoundName;
	AmbientInfo.Location = Location;
	AmbientInfo.Volume = Volume;
	AmbientInfo.bIsActive = true;

	// Spawn audio component
	AmbientInfo.AudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		GetWorld(),
		SoundCue,
		Location,
		FRotator::ZeroRotator,
		Volume * AudioSettings.AmbientVolume * AudioSettings.MasterVolume,
		1.0f,
		0.0f,
		nullptr,
		nullptr,
		false // Don't auto-destroy
	);

	if (AmbientInfo.AudioComponent)
	{
		AmbientInfo.AudioComponent->bAllowSpatialization = true;
		AmbientSounds.Add(SoundName, AmbientInfo);

		// Add to active components
		if (ActiveAudioComponents.Contains(EAudioLayer::Ambient))
		{
			ActiveAudioComponents[EAudioLayer::Ambient].Add(AmbientInfo.AudioComponent);
		}
	}
}

void UAdvancedAudioSystem::RemoveAmbientSound(const FString& SoundName)
{
	if (AmbientSounds.Contains(SoundName))
	{
		FAmbientSoundInfo& AmbientInfo = AmbientSounds[SoundName];
		if (AmbientInfo.AudioComponent && IsValid(AmbientInfo.AudioComponent))
		{
			AmbientInfo.AudioComponent->Stop();
			
			// Remove from active components
			if (ActiveAudioComponents.Contains(EAudioLayer::Ambient))
			{
				ActiveAudioComponents[EAudioLayer::Ambient].Remove(AmbientInfo.AudioComponent);
			}
		}
		AmbientSounds.Remove(SoundName);
	}
}

void UAdvancedAudioSystem::SetAmbientSoundVolume(const FString& SoundName, float Volume)
{
	if (AmbientSounds.Contains(SoundName))
	{
		FAmbientSoundInfo& AmbientInfo = AmbientSounds[SoundName];
		AmbientInfo.Volume = Volume;
		
		if (AmbientInfo.AudioComponent && IsValid(AmbientInfo.AudioComponent))
		{
			AmbientInfo.AudioComponent->SetVolumeMultiplier(
				Volume * AudioSettings.AmbientVolume * AudioSettings.MasterVolume
			);
		}
	}
}

void UAdvancedAudioSystem::SetAmbientSoundLocation(const FString& SoundName, const FVector& NewLocation)
{
	if (AmbientSounds.Contains(SoundName))
	{
		FAmbientSoundInfo& AmbientInfo = AmbientSounds[SoundName];
		AmbientInfo.Location = NewLocation;
		
		if (AmbientInfo.AudioComponent && IsValid(AmbientInfo.AudioComponent))
		{
			AmbientInfo.AudioComponent->SetWorldLocation(NewLocation);
		}
	}
}

void UAdvancedAudioSystem::SetAudioEnvironment(EAudioEnvironment Environment)
{
	CurrentEnvironment = Environment;

	// Apply environmental settings to all active audio components
	// This would typically involve setting reverb parameters
	// Implementation depends on your audio middleware (FMOD, Wwise, etc.)
	
	for (auto& LayerPair : ActiveAudioComponents)
	{
		for (UAudioComponent* AudioComp : LayerPair.Value)
		{
			if (AudioComp && IsValid(AudioComp))
			{
				ProcessEnvironmentalEffects(AudioComp);
			}
		}
	}
}

void UAdvancedAudioSystem::PlayWeaponSound(USoundCue* WeaponSound, const FVector& Location, float VolumeMultiplier)
{
	if (!WeaponSound)
	{
		return;
	}

	FAudioCue AudioCue;
	AudioCue.SoundCue = WeaponSound;
	AudioCue.AudioLayer = EAudioLayer::Weapon;
	AudioCue.Volume = VolumeMultiplier;
	AudioCue.MinDistance = 100.0f;
	AudioCue.MaxDistance = 2000.0f;
	AudioCue.bIs3D = true;
	AudioCue.bAutoDestroy = true;

	PlaySoundAtLocation(AudioCue, Location);
}

void UAdvancedAudioSystem::PlayWeaponSoundWithEcho(USoundCue* WeaponSound, const FVector& Location, float EchoDelay, float EchoVolume)
{
	// Play primary weapon sound
	PlayWeaponSound(WeaponSound, Location, 1.0f);

	// Schedule echo sound
	FTimerHandle EchoTimer;
	GetWorld()->GetTimerManager().SetTimer(EchoTimer, [this, WeaponSound, Location, EchoVolume]()
	{
		PlayWeaponSound(WeaponSound, Location, EchoVolume);
	}, EchoDelay, false);
}

void UAdvancedAudioSystem::StopAllSounds()
{
	for (auto& LayerPair : ActiveAudioComponents)
	{
		for (UAudioComponent* AudioComp : LayerPair.Value)
		{
			if (AudioComp && IsValid(AudioComp))
			{
				AudioComp->Stop();
			}
		}
		LayerPair.Value.Empty();
	}

	// Stop music
	StopMusic(0.0f);

	// Stop ambient sounds
	for (auto& AmbientPair : AmbientSounds)
	{
		if (AmbientPair.Value.AudioComponent && IsValid(AmbientPair.Value.AudioComponent))
		{
			AmbientPair.Value.AudioComponent->Stop();
		}
	}
	AmbientSounds.Empty();
}

void UAdvancedAudioSystem::StopSoundsByLayer(EAudioLayer Layer)
{
	if (ActiveAudioComponents.Contains(Layer))
	{
		for (UAudioComponent* AudioComp : ActiveAudioComponents[Layer])
		{
			if (AudioComp && IsValid(AudioComp))
			{
				AudioComp->Stop();
			}
		}
		ActiveAudioComponents[Layer].Empty();
	}
}

void UAdvancedAudioSystem::FadeAllSounds(float FadeTime, float TargetVolume)
{
	for (auto& LayerPair : ActiveAudioComponents)
	{
		for (UAudioComponent* AudioComp : LayerPair.Value)
		{
			if (AudioComp && IsValid(AudioComp))
			{
				AudioComp->FadeOut(FadeTime, TargetVolume);
			}
		}
	}
}

float UAdvancedAudioSystem::CalculateDistanceAttenuation(const FVector& SoundLocation, const FVector& ListenerLocation, float MinDistance, float MaxDistance) const
{
	float Distance = FVector::Dist(SoundLocation, ListenerLocation);
	
	if (Distance <= MinDistance)
	{
		return 1.0f;
	}
	else if (Distance >= MaxDistance)
	{
		return 0.0f;
	}
	else
	{
		// Linear attenuation
		return 1.0f - ((Distance - MinDistance) / (MaxDistance - MinDistance));
	}
}

bool UAdvancedAudioSystem::IsLocationOccluded(const FVector& SoundLocation, const FVector& ListenerLocation) const
{
	if (!GetWorld())
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		SoundLocation,
		ListenerLocation,
		ECC_Visibility,
		QueryParams
	);

	return bHit;
}

void UAdvancedAudioSystem::UpdateListenerLocation(const FVector& Location, const FRotator& Rotation)
{
	LastListenerLocation = Location;
	LastListenerRotation = Rotation;

	// Update audio listener in engine
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		UGameplayStatics::SetGlobalListenerFocusParameters(GetWorld(), 1.0f, 1.0f, 1.0f, 1.0f);
	}
}

// Voice chat functions
void UAdvancedAudioSystem::SetVoiceChatEnabled(bool bEnabled)
{
	bVoiceChatEnabled = bEnabled;
}

void UAdvancedAudioSystem::SetVoiceChatVolume(float Volume)
{
	VoiceChatVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
}

void UAdvancedAudioSystem::MutePlayer(const FString& PlayerName)
{
	MutedPlayers.Add(PlayerName);
}

void UAdvancedAudioSystem::UnmutePlayer(const FString& PlayerName)
{
	MutedPlayers.Remove(PlayerName);
}

// Protected helper functions
void UAdvancedAudioSystem::ProcessOcclusion(UAudioComponent* AudioComponent, const FVector& SoundLocation)
{
	if (!AudioComponent || !IsValid(AudioComponent))
	{
		return;
	}

	bool bIsOccluded = IsLocationOccluded(SoundLocation, LastListenerLocation);
	
	if (bIsOccluded)
	{
		// Apply occlusion effect (reduce volume and apply low-pass filter)
		float OcclusionFactor = 0.3f; // Configurable
		AudioComponent->SetVolumeMultiplier(AudioComponent->GetVolumeMultiplier() * OcclusionFactor);
		
		// Apply low-pass filter effect if available
		// This would typically be done through audio middleware
	}
}

void UAdvancedAudioSystem::ProcessDopplerEffect(UAudioComponent* AudioComponent, const FVector& SoundLocation, const FVector& SoundVelocity)
{
	if (!AudioComponent || !IsValid(AudioComponent))
	{
		return;
	}

	// Calculate Doppler shift
	FVector ToListener = LastListenerLocation - SoundLocation;
	ToListener.Normalize();

	float RelativeVelocity = FVector::DotProduct(SoundVelocity - ListenerVelocity, ToListener);
	float SpeedOfSound = 343.0f; // m/s

	float DopplerFactor = (SpeedOfSound) / (SpeedOfSound - RelativeVelocity);
	DopplerFactor = FMath::Clamp(DopplerFactor, 0.5f, 2.0f); // Reasonable limits

	AudioComponent->SetPitchMultiplier(DopplerFactor);
}

void UAdvancedAudioSystem::ProcessEnvironmentalEffects(UAudioComponent* AudioComponent)
{
	if (!AudioComponent || !IsValid(AudioComponent))
	{
		return;
	}

	// Apply reverb and environmental effects based on current environment
	// This would typically involve setting reverb parameters through audio middleware
	switch (CurrentEnvironment)
	{
		case EAudioEnvironment::Cave:
			// High reverb, long decay
			break;
		case EAudioEnvironment::SmallRoom:
			// Medium reverb, short decay
			break;
		case EAudioEnvironment::Underwater:
			// Heavy filtering, unique reverb
			break;
		// Add more environments as needed
		default:
			break;
	}
}

void UAdvancedAudioSystem::UpdateMusicCrossfade(float DeltaTime)
{
	if (!bIsCrossfading)
	{
		return;
	}

	CrossfadeTimer += DeltaTime;
	float Progress = CrossfadeTimer / CrossfadeDuration;

	if (Progress >= 1.0f)
	{
		// Crossfade complete
		Progress = 1.0f;
		bIsCrossfading = false;

		// Stop and clean up previous music
		if (PreviousMusicComponent && IsValid(PreviousMusicComponent))
		{
			PreviousMusicComponent->Stop();
			PreviousMusicComponent = nullptr;
		}
	}

	// Update volumes
	if (CurrentMusicComponent && IsValid(CurrentMusicComponent))
	{
		float NewMusicVolume = Progress * AudioSettings.MasterVolume * AudioSettings.MusicVolume;
		CurrentMusicComponent->SetVolumeMultiplier(NewMusicVolume);
	}

	if (PreviousMusicComponent && IsValid(PreviousMusicComponent))
	{
		float OldMusicVolume = (1.0f - Progress) * AudioSettings.MasterVolume * AudioSettings.MusicVolume;
		PreviousMusicComponent->SetVolumeMultiplier(OldMusicVolume);
	}
}

void UAdvancedAudioSystem::CleanupFinishedAudioComponents()
{
	for (auto& LayerPair : ActiveAudioComponents)
	{
		TArray<UAudioComponent*>& Components = LayerPair.Value;
		
		for (int32 i = Components.Num() - 1; i >= 0; i--)
		{
			UAudioComponent* AudioComp = Components[i];
			if (!AudioComp || !IsValid(AudioComp) || !AudioComp->IsPlaying())
			{
				Components.RemoveAt(i);
			}
		}
	}
}

float UAdvancedAudioSystem::GetVolumeForLayer(EAudioLayer Layer) const
{
	float LayerVolume = 1.0f;
	
	switch (Layer)
	{
		case EAudioLayer::Master:
			LayerVolume = 1.0f;
			break;
		case EAudioLayer::Music:
			LayerVolume = AudioSettings.MusicVolume;
			break;
		case EAudioLayer::SFX:
			LayerVolume = AudioSettings.SFXVolume;
			break;
		case EAudioLayer::Voice:
			LayerVolume = AudioSettings.VoiceVolume;
			break;
		case EAudioLayer::Ambient:
			LayerVolume = AudioSettings.AmbientVolume;
			break;
		case EAudioLayer::UI:
			LayerVolume = AudioSettings.UIVolume;
			break;
		case EAudioLayer::Weapon:
			LayerVolume = AudioSettings.WeaponVolume;
			break;
	}

	return LayerVolume * AudioSettings.MasterVolume;
}

USoundAttenuation* UAdvancedAudioSystem::GetAttenuationSettingsForEnvironment(EAudioEnvironment Environment)
{
	// This would return different attenuation settings based on environment
	// For now, return nullptr to use default settings
	return nullptr;
}

void UAdvancedAudioSystem::ApplyLayerVolumeToComponent(UAudioComponent* AudioComponent, EAudioLayer Layer)
{
	if (AudioComponent && IsValid(AudioComponent))
	{
		float LayerVolume = GetVolumeForLayer(Layer);
		AudioComponent->SetVolumeMultiplier(LayerVolume);
	}
}
