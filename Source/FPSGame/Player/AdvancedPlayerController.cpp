#include "AdvancedPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

DEFINE_LOG_CATEGORY(LogAdvancedPlayerController);

AAdvancedPlayerController::AAdvancedPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    
    // Initialize Enhanced Input
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    
    // Default settings
    MouseSensitivity = 1.0f;
    FieldOfView = 90.0f;
    MasterVolume = 1.0f;
    SFXVolume = 1.0f;
    MusicVolume = 1.0f;
    VoiceChatVolume = 1.0f;
    
    bInvertMouseY = false;
    bEnableVSync = true;
    bShowFPS = false;
    bEnableVoiceChat = true;
    bPushToTalk = true;
    
    CurrentSpectatorMode = ESpectatorMode::None;
    SpectatedPlayerIndex = 0;
    
    // Initialize statistics
    SessionStatistics.SessionStartTime = 0.0f;
    SessionStatistics.Kills = 0;
    SessionStatistics.Deaths = 0;
    SessionStatistics.Assists = 0;
    SessionStatistics.Score = 0;
    SessionStatistics.DamageDealt = 0.0f;
    SessionStatistics.DamageTaken = 0.0f;
    SessionStatistics.ShotsFired = 0;
    SessionStatistics.ShotsHit = 0;
    SessionStatistics.HeadshotKills = 0;
    SessionStatistics.Accuracy = 0.0f;
    SessionStatistics.KillDeathRatio = 0.0f;
    
    bIsAdministrator = false;
    AdminLevel = 0;
    
    // Create input component
    InputComponent = CreateDefaultSubobject<UEnhancedInputComponent>(TEXT("EnhancedInputComponent"));
}

void AAdvancedPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize Enhanced Input
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (InputMappingContext)
        {
            Subsystem->AddMappingContext(InputMappingContext, 0);
        }
    }
    
    // Load settings
    LoadPlayerSettings();
    
    // Initialize session statistics
    SessionStatistics.SessionStartTime = GetWorld()->GetTimeSeconds();
    
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Advanced Player Controller initialized"));
}

void AAdvancedPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Update statistics
    UpdateSessionStatistics(DeltaTime);
    
    // Handle spectator movement
    if (CurrentSpectatorMode != ESpectatorMode::None)
    {
        UpdateSpectatorCamera(DeltaTime);
    }
    
    // Update voice chat
    UpdateVoiceChat(DeltaTime);
}

void AAdvancedPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // Movement
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAdvancedPlayerController::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAdvancedPlayerController::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAdvancedPlayerController::StopJumping);
        
        // Combat
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::StartFire);
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AAdvancedPlayerController::StopFire);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::StartAim);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AAdvancedPlayerController::StopAim);
        EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::Reload);
        
        // Movement modes
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::StartSprint);
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AAdvancedPlayerController::StopSprint);
        EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::ToggleCrouch);
        EnhancedInputComponent->BindAction(ProneAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::ToggleProne);
        
        // Interaction
        EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::Interact);
        EnhancedInputComponent->BindAction(UseAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::Use);
        
        // Communication
        EnhancedInputComponent->BindAction(VoiceChatAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::StartVoiceChat);
        EnhancedInputComponent->BindAction(VoiceChatAction, ETriggerEvent::Completed, this, &AAdvancedPlayerController::StopVoiceChat);
        EnhancedInputComponent->BindAction(ChatAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::OpenTextChat);
        
        // UI
        EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::ToggleMenu);
        EnhancedInputComponent->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::ShowScoreboard);
        EnhancedInputComponent->BindAction(ScoreboardAction, ETriggerEvent::Completed, this, &AAdvancedPlayerController::HideScoreboard);
        
        // Weapon switching
        EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::SwitchWeapon);
        EnhancedInputComponent->BindAction(NextWeaponAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::NextWeapon);
        EnhancedInputComponent->BindAction(PreviousWeaponAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::PreviousWeapon);
        
        // Spectator controls
        EnhancedInputComponent->BindAction(SpectatorNextAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::SpectatorNext);
        EnhancedInputComponent->BindAction(SpectatorPreviousAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::SpectatorPrevious);
        EnhancedInputComponent->BindAction(SpectatorModeAction, ETriggerEvent::Started, this, &AAdvancedPlayerController::CycleSpectatorMode);
    }
}

void AAdvancedPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AAdvancedPlayerController, SessionStatistics);
    DOREPLIFETIME(AAdvancedPlayerController, bIsAdministrator);
    DOREPLIFETIME(AAdvancedPlayerController, AdminLevel);
    DOREPLIFETIME(AAdvancedPlayerController, CurrentSpectatorMode);
}

void AAdvancedPlayerController::Move(const FInputActionValue& Value)
{
    if (CurrentSpectatorMode != ESpectatorMode::None)
    {
        SpectatorMove(Value);
        return;
    }
    
    const FVector2D MovementVector = Value.Get<FVector2D>();
    
    if (APawn* ControlledPawn = GetPawn())
    {
        // Forward/Backward movement
        ControlledPawn->AddMovementInput(ControlledPawn->GetActorForwardVector(), MovementVector.Y);
        
        // Left/Right movement
        ControlledPawn->AddMovementInput(ControlledPawn->GetActorRightVector(), MovementVector.X);
    }
}

void AAdvancedPlayerController::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    
    if (CurrentSpectatorMode != ESpectatorMode::None)
    {
        SpectatorLook(LookAxisVector);
        return;
    }
    
    // Apply mouse sensitivity
    FVector2D ScaledInput = LookAxisVector * MouseSensitivity;
    
    // Invert Y axis if enabled
    if (bInvertMouseY)
    {
        ScaledInput.Y *= -1.0f;
    }
    
    // Add input to the controller
    AddYawInput(ScaledInput.X);
    AddPitchInput(ScaledInput.Y);
}

void AAdvancedPlayerController::Jump()
{
    if (ACharacter* Character = Cast<ACharacter>(GetPawn()))
    {
        Character->Jump();
    }
}

void AAdvancedPlayerController::StopJumping()
{
    if (ACharacter* Character = Cast<ACharacter>(GetPawn()))
    {
        Character->StopJumping();
    }
}

void AAdvancedPlayerController::StartFire()
{
    // Call server fire function
    ServerStartFire();
}

void AAdvancedPlayerController::StopFire()
{
    ServerStopFire();
}

void AAdvancedPlayerController::ServerStartFire_Implementation()
{
    // Implement weapon firing logic
    if (APawn* ControlledPawn = GetPawn())
    {
        // This would interface with your weapon system
        OnFireStarted.Broadcast();
    }
}

void AAdvancedPlayerController::ServerStopFire_Implementation()
{
    OnFireStopped.Broadcast();
}

bool AAdvancedPlayerController::ServerStartFire_Validate()
{
    return true;
}

bool AAdvancedPlayerController::ServerStopFire_Validate()
{
    return true;
}

void AAdvancedPlayerController::StartAim()
{
    ServerStartAim();
}

void AAdvancedPlayerController::StopAim()
{
    ServerStopAim();
}

void AAdvancedPlayerController::ServerStartAim_Implementation()
{
    OnAimStarted.Broadcast();
}

void AAdvancedPlayerController::ServerStopAim_Implementation()
{
    OnAimStopped.Broadcast();
}

void AAdvancedPlayerController::Reload()
{
    ServerReload();
}

void AAdvancedPlayerController::ServerReload_Implementation()
{
    OnReloadRequested.Broadcast();
}

void AAdvancedPlayerController::StartSprint()
{
    ServerStartSprint();
}

void AAdvancedPlayerController::StopSprint()
{
    ServerStopSprint();
}

void AAdvancedPlayerController::ServerStartSprint_Implementation()
{
    if (ACharacter* Character = Cast<ACharacter>(GetPawn()))
    {
        // Increase movement speed
        OnSprintStarted.Broadcast();
    }
}

void AAdvancedPlayerController::ServerStopSprint_Implementation()
{
    OnSprintStopped.Broadcast();
}

void AAdvancedPlayerController::ToggleCrouch()
{
    ServerToggleCrouch();
}

void AAdvancedPlayerController::ServerToggleCrouch_Implementation()
{
    if (ACharacter* Character = Cast<ACharacter>(GetPawn()))
    {
        if (Character->bIsCrouched)
        {
            Character->UnCrouch();
        }
        else
        {
            Character->Crouch();
        }
    }
}

void AAdvancedPlayerController::ToggleProne()
{
    ServerToggleProne();
}

void AAdvancedPlayerController::ServerToggleProne_Implementation()
{
    OnProneToggled.Broadcast();
}

void AAdvancedPlayerController::Interact()
{
    ServerInteract();
}

void AAdvancedPlayerController::ServerInteract_Implementation()
{
    OnInteractRequested.Broadcast();
}

void AAdvancedPlayerController::Use()
{
    ServerUse();
}

void AAdvancedPlayerController::ServerUse_Implementation()
{
    OnUseRequested.Broadcast();
}

void AAdvancedPlayerController::StartVoiceChat()
{
    if (bEnableVoiceChat)
    {
        bIsTransmittingVoice = true;
        OnVoiceChatStarted.Broadcast();
        
        UE_LOG(LogAdvancedPlayerController, Log, TEXT("Voice chat started"));
    }
}

void AAdvancedPlayerController::StopVoiceChat()
{
    bIsTransmittingVoice = false;
    OnVoiceChatStopped.Broadcast();
    
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Voice chat stopped"));
}

void AAdvancedPlayerController::OpenTextChat()
{
    OnTextChatRequested.Broadcast();
}

void AAdvancedPlayerController::SendChatMessage(const FString& Message, bool bTeamOnly)
{
    ServerSendChatMessage(Message, bTeamOnly);
}

void AAdvancedPlayerController::ServerSendChatMessage_Implementation(const FString& Message, bool bTeamOnly)
{
    // Broadcast message to appropriate players
    FChatMessage ChatMessage;
    ChatMessage.PlayerName = GetPlayerState<APlayerState>()->GetPlayerName();
    ChatMessage.Message = Message;
    ChatMessage.bIsTeamMessage = bTeamOnly;
    ChatMessage.Timestamp = GetWorld()->GetTimeSeconds();
    
    MulticastReceiveChatMessage(ChatMessage);
}

void AAdvancedPlayerController::MulticastReceiveChatMessage_Implementation(const FChatMessage& ChatMessage)
{
    OnChatMessageReceived.Broadcast(ChatMessage);
}

void AAdvancedPlayerController::ToggleMenu()
{
    OnMenuToggled.Broadcast();
}

void AAdvancedPlayerController::ShowScoreboard()
{
    OnScoreboardShown.Broadcast();
}

void AAdvancedPlayerController::HideScoreboard()
{
    OnScoreboardHidden.Broadcast();
}

void AAdvancedPlayerController::SwitchWeapon(const FInputActionValue& Value)
{
    int32 WeaponIndex = FMath::RoundToInt(Value.Get<float>());
    ServerSwitchWeapon(WeaponIndex);
}

void AAdvancedPlayerController::NextWeapon()
{
    ServerNextWeapon();
}

void AAdvancedPlayerController::PreviousWeapon()
{
    ServerPreviousWeapon();
}

void AAdvancedPlayerController::ServerSwitchWeapon_Implementation(int32 WeaponIndex)
{
    OnWeaponSwitchRequested.Broadcast(WeaponIndex);
}

void AAdvancedPlayerController::ServerNextWeapon_Implementation()
{
    OnNextWeaponRequested.Broadcast();
}

void AAdvancedPlayerController::ServerPreviousWeapon_Implementation()
{
    OnPreviousWeaponRequested.Broadcast();
}

void AAdvancedPlayerController::EnterSpectatorMode(ESpectatorMode Mode)
{
    if (CurrentSpectatorMode == Mode) return;
    
    CurrentSpectatorMode = Mode;
    SpectatedPlayerIndex = 0;
    
    // Set appropriate pawn class
    switch (Mode)
    {
        case ESpectatorMode::FreeCamera:
            ChangeState(NAME_Spectating);
            break;
            
        case ESpectatorMode::FollowPlayer:
            FindNextSpectatorTarget();
            break;
            
        case ESpectatorMode::FixedCamera:
            // Set to fixed camera position
            break;
            
        default:
            break;
    }
    
    OnSpectatorModeChanged.Broadcast(Mode);
    
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Entered spectator mode: %s"), 
           *UEnum::GetValueAsString(Mode));
}

void AAdvancedPlayerController::ExitSpectatorMode()
{
    if (CurrentSpectatorMode == ESpectatorMode::None) return;
    
    CurrentSpectatorMode = ESpectatorMode::None;
    
    // Request respawn
    OnRespawnRequested.Broadcast();
    
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Exited spectator mode"));
}

void AAdvancedPlayerController::SpectatorNext()
{
    if (CurrentSpectatorMode == ESpectatorMode::FollowPlayer)
    {
        FindNextSpectatorTarget();
    }
    else if (CurrentSpectatorMode == ESpectatorMode::FixedCamera)
    {
        // Switch to next fixed camera
    }
}

void AAdvancedPlayerController::SpectatorPrevious()
{
    if (CurrentSpectatorMode == ESpectatorMode::FollowPlayer)
    {
        FindPreviousSpectatorTarget();
    }
    else if (CurrentSpectatorMode == ESpectatorMode::FixedCamera)
    {
        // Switch to previous fixed camera
    }
}

void AAdvancedPlayerController::CycleSpectatorMode()
{
    switch (CurrentSpectatorMode)
    {
        case ESpectatorMode::None:
            EnterSpectatorMode(ESpectatorMode::FreeCamera);
            break;
        case ESpectatorMode::FreeCamera:
            EnterSpectatorMode(ESpectatorMode::FollowPlayer);
            break;
        case ESpectatorMode::FollowPlayer:
            EnterSpectatorMode(ESpectatorMode::FixedCamera);
            break;
        case ESpectatorMode::FixedCamera:
            EnterSpectatorMode(ESpectatorMode::FreeCamera);
            break;
    }
}

void AAdvancedPlayerController::FindNextSpectatorTarget()
{
    TArray<APawn*> AlivePlayers;
    
    // Find all alive players
    for (FConstPawnIterator Iterator = GetWorld()->GetPawnIterator(); Iterator; ++Iterator)
    {
        if (APawn* Pawn = Iterator->Get())
        {
            if (Pawn->IsValidLowLevelFast() && !Pawn->IsPendingKill() && Pawn != GetPawn())
            {
                AlivePlayers.Add(Pawn);
            }
        }
    }
    
    if (AlivePlayers.Num() > 0)
    {
        SpectatedPlayerIndex = (SpectatedPlayerIndex + 1) % AlivePlayers.Num();
        APawn* TargetPawn = AlivePlayers[SpectatedPlayerIndex];
        
        SetViewTarget(TargetPawn);
        
        UE_LOG(LogAdvancedPlayerController, Log, TEXT("Spectating player: %s"), 
               *TargetPawn->GetName());
    }
}

void AAdvancedPlayerController::FindPreviousSpectatorTarget()
{
    TArray<APawn*> AlivePlayers;
    
    // Find all alive players
    for (FConstPawnIterator Iterator = GetWorld()->GetPawnIterator(); Iterator; ++Iterator)
    {
        if (APawn* Pawn = Iterator->Get())
        {
            if (Pawn->IsValidLowLevelFast() && !Pawn->IsPendingKill() && Pawn != GetPawn())
            {
                AlivePlayers.Add(Pawn);
            }
        }
    }
    
    if (AlivePlayers.Num() > 0)
    {
        SpectatedPlayerIndex--;
        if (SpectatedPlayerIndex < 0)
        {
            SpectatedPlayerIndex = AlivePlayers.Num() - 1;
        }
        
        APawn* TargetPawn = AlivePlayers[SpectatedPlayerIndex];
        SetViewTarget(TargetPawn);
        
        UE_LOG(LogAdvancedPlayerController, Log, TEXT("Spectating player: %s"), 
               *TargetPawn->GetName());
    }
}

void AAdvancedPlayerController::SpectatorMove(const FInputActionValue& Value)
{
    if (CurrentSpectatorMode != ESpectatorMode::FreeCamera) return;
    
    const FVector2D MovementVector = Value.Get<FVector2D>();
    
    if (APawn* SpectatorPawn = GetSpectatorPawn())
    {
        SpectatorPawn->AddMovementInput(SpectatorPawn->GetActorForwardVector(), MovementVector.Y);
        SpectatorPawn->AddMovementInput(SpectatorPawn->GetActorRightVector(), MovementVector.X);
    }
}

void AAdvancedPlayerController::SpectatorLook(const FVector2D& LookInput)
{
    if (CurrentSpectatorMode != ESpectatorMode::FreeCamera) return;
    
    FVector2D ScaledInput = LookInput * MouseSensitivity;
    
    if (bInvertMouseY)
    {
        ScaledInput.Y *= -1.0f;
    }
    
    AddYawInput(ScaledInput.X);
    AddPitchInput(ScaledInput.Y);
}

void AAdvancedPlayerController::UpdateSpectatorCamera(float DeltaTime)
{
    // Update spectator camera logic based on mode
    switch (CurrentSpectatorMode)
    {
        case ESpectatorMode::FollowPlayer:
            UpdateFollowPlayerCamera(DeltaTime);
            break;
            
        case ESpectatorMode::FixedCamera:
            UpdateFixedCamera(DeltaTime);
            break;
            
        default:
            break;
    }
}

void AAdvancedPlayerController::UpdateFollowPlayerCamera(float DeltaTime)
{
    // Smooth camera following for spectated player
    if (AActor* ViewTarget = GetViewTarget())
    {
        // Add camera offset and smooth movement
    }
}

void AAdvancedPlayerController::UpdateFixedCamera(float DeltaTime)
{
    // Update fixed camera positions
}

void AAdvancedPlayerController::UpdateVoiceChat(float DeltaTime)
{
    if (bIsTransmittingVoice && bEnableVoiceChat)
    {
        // Update voice transmission
        ProcessVoiceData();
    }
}

void AAdvancedPlayerController::ProcessVoiceData()
{
    // Process and transmit voice data
    // This would interface with your voice chat system
}

void AAdvancedPlayerController::UpdateSessionStatistics(float DeltaTime)
{
    // Update session time
    SessionStatistics.SessionTime = GetWorld()->GetTimeSeconds() - SessionStatistics.SessionStartTime;
    
    // Calculate derived statistics
    if (SessionStatistics.Deaths > 0)
    {
        SessionStatistics.KillDeathRatio = (float)SessionStatistics.Kills / (float)SessionStatistics.Deaths;
    }
    else
    {
        SessionStatistics.KillDeathRatio = (float)SessionStatistics.Kills;
    }
    
    if (SessionStatistics.ShotsFired > 0)
    {
        SessionStatistics.Accuracy = (float)SessionStatistics.ShotsHit / (float)SessionStatistics.ShotsFired;
    }
}

void AAdvancedPlayerController::AddKill(bool bHeadshot)
{
    SessionStatistics.Kills++;
    if (bHeadshot)
    {
        SessionStatistics.HeadshotKills++;
    }
    
    OnStatisticsUpdated.Broadcast();
}

void AAdvancedPlayerController::AddDeath()
{
    SessionStatistics.Deaths++;
    OnStatisticsUpdated.Broadcast();
}

void AAdvancedPlayerController::AddAssist()
{
    SessionStatistics.Assists++;
    OnStatisticsUpdated.Broadcast();
}

void AAdvancedPlayerController::AddScore(int32 Points)
{
    SessionStatistics.Score += Points;
    OnStatisticsUpdated.Broadcast();
}

void AAdvancedPlayerController::AddDamageDealt(float Damage)
{
    SessionStatistics.DamageDealt += Damage;
    OnStatisticsUpdated.Broadcast();
}

void AAdvancedPlayerController::AddDamageTaken(float Damage)
{
    SessionStatistics.DamageTaken += Damage;
    OnStatisticsUpdated.Broadcast();
}

void AAdvancedPlayerController::AddShotFired()
{
    SessionStatistics.ShotsFired++;
}

void AAdvancedPlayerController::AddShotHit()
{
    SessionStatistics.ShotsHit++;
}

void AAdvancedPlayerController::ApplySettings(const FPlayerSettings& Settings)
{
    MouseSensitivity = Settings.MouseSensitivity;
    FieldOfView = Settings.FieldOfView;
    bInvertMouseY = Settings.bInvertMouseY;
    
    MasterVolume = Settings.MasterVolume;
    SFXVolume = Settings.SFXVolume;
    MusicVolume = Settings.MusicVolume;
    VoiceChatVolume = Settings.VoiceChatVolume;
    
    bEnableVSync = Settings.bEnableVSync;
    bShowFPS = Settings.bShowFPS;
    bEnableVoiceChat = Settings.bEnableVoiceChat;
    bPushToTalk = Settings.bPushToTalk;
    
    // Apply graphics settings
    ApplyGraphicsSettings(Settings.GraphicsSettings);
    
    // Apply audio settings
    ApplyAudioSettings();
    
    // Save settings
    SavePlayerSettings();
    
    OnSettingsChanged.Broadcast(Settings);
    
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Player settings applied"));
}

void AAdvancedPlayerController::ApplyGraphicsSettings(const FGraphicsSettings& GraphicsSettings)
{
    // Apply graphics quality settings
    if (UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings())
    {
        GameUserSettings->SetTextureQuality(GraphicsSettings.TextureQuality);
        GameUserSettings->SetShadowQuality(GraphicsSettings.ShadowQuality);
        GameUserSettings->SetPostProcessingQuality(GraphicsSettings.PostProcessQuality);
        GameUserSettings->SetEffectsQuality(GraphicsSettings.EffectsQuality);
        GameUserSettings->SetAntiAliasingQuality(GraphicsSettings.AntiAliasingQuality);
        GameUserSettings->SetVSyncEnabled(bEnableVSync);
        
        GameUserSettings->ApplySettings(false);
    }
}

void AAdvancedPlayerController::ApplyAudioSettings()
{
    // Apply audio volume settings
    if (UAudioDevice* AudioDevice = GetWorld()->GetAudioDevice())
    {
        // Set master volume
        AudioDevice->SetTransientMasterVolume(MasterVolume);
    }
}

void AAdvancedPlayerController::SavePlayerSettings()
{
    // Save settings to file or user preferences
    if (UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings())
    {
        GameUserSettings->SaveSettings();
    }
}

void AAdvancedPlayerController::LoadPlayerSettings()
{
    // Load settings from file or user preferences
    if (UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings())
    {
        GameUserSettings->LoadSettings();
        
        // Apply loaded settings
        FPlayerSettings LoadedSettings;
        LoadedSettings.bEnableVSync = GameUserSettings->IsVSyncEnabled();
        LoadedSettings.GraphicsSettings.TextureQuality = GameUserSettings->GetTextureQuality();
        LoadedSettings.GraphicsSettings.ShadowQuality = GameUserSettings->GetShadowQuality();
        LoadedSettings.GraphicsSettings.PostProcessQuality = GameUserSettings->GetPostProcessingQuality();
        LoadedSettings.GraphicsSettings.EffectsQuality = GameUserSettings->GetEffectsQuality();
        LoadedSettings.GraphicsSettings.AntiAliasingQuality = GameUserSettings->GetAntiAliasingQuality();
        
        ApplySettings(LoadedSettings);
    }
}

FPlayerSettings AAdvancedPlayerController::GetCurrentSettings() const
{
    FPlayerSettings Settings;
    
    Settings.MouseSensitivity = MouseSensitivity;
    Settings.FieldOfView = FieldOfView;
    Settings.bInvertMouseY = bInvertMouseY;
    
    Settings.MasterVolume = MasterVolume;
    Settings.SFXVolume = SFXVolume;
    Settings.MusicVolume = MusicVolume;
    Settings.VoiceChatVolume = VoiceChatVolume;
    
    Settings.bEnableVSync = bEnableVSync;
    Settings.bShowFPS = bShowFPS;
    Settings.bEnableVoiceChat = bEnableVoiceChat;
    Settings.bPushToTalk = bPushToTalk;
    
    // Get graphics settings
    if (UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings())
    {
        Settings.GraphicsSettings.TextureQuality = GameUserSettings->GetTextureQuality();
        Settings.GraphicsSettings.ShadowQuality = GameUserSettings->GetShadowQuality();
        Settings.GraphicsSettings.PostProcessQuality = GameUserSettings->GetPostProcessingQuality();
        Settings.GraphicsSettings.EffectsQuality = GameUserSettings->GetEffectsQuality();
        Settings.GraphicsSettings.AntiAliasingQuality = GameUserSettings->GetAntiAliasingQuality();
    }
    
    return Settings;
}

void AAdvancedPlayerController::ExecuteAdminCommand(const FString& Command)
{
    if (!bIsAdministrator)
    {
        UE_LOG(LogAdvancedPlayerController, Warning, TEXT("Player attempted admin command without privileges"));
        return;
    }
    
    ServerExecuteAdminCommand(Command);
}

void AAdvancedPlayerController::ServerExecuteAdminCommand_Implementation(const FString& Command)
{
    if (!bIsAdministrator) return;
    
    // Parse and execute admin command
    TArray<FString> CommandParts;
    Command.ParseIntoArray(CommandParts, TEXT(" "), true);
    
    if (CommandParts.Num() == 0) return;
    
    FString CommandName = CommandParts[0].ToLower();
    
    if (CommandName == TEXT("kick"))
    {
        if (CommandParts.Num() > 1)
        {
            KickPlayer(CommandParts[1]);
        }
    }
    else if (CommandName == TEXT("ban"))
    {
        if (CommandParts.Num() > 1)
        {
            BanPlayer(CommandParts[1]);
        }
    }
    else if (CommandName == TEXT("changemap"))
    {
        if (CommandParts.Num() > 1)
        {
            ChangeMap(CommandParts[1]);
        }
    }
    
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Admin command executed: %s"), *Command);
}

bool AAdvancedPlayerController::ServerExecuteAdminCommand_Validate(const FString& Command)
{
    return bIsAdministrator;
}

void AAdvancedPlayerController::KickPlayer(const FString& PlayerName)
{
    // Implementation would kick the specified player
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Kicking player: %s"), *PlayerName);
}

void AAdvancedPlayerController::BanPlayer(const FString& PlayerName)
{
    // Implementation would ban the specified player
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Banning player: %s"), *PlayerName);
}

void AAdvancedPlayerController::ChangeMap(const FString& MapName)
{
    // Implementation would change to the specified map
    UGameplayStatics::OpenLevel(GetWorld(), FName(*MapName));
    UE_LOG(LogAdvancedPlayerController, Log, TEXT("Changing map to: %s"), *MapName);
}

FPlayerStatistics AAdvancedPlayerController::GetSessionStatistics() const
{
    return SessionStatistics;
}
