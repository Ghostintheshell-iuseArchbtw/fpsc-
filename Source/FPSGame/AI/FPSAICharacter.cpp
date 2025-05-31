#include "FPSAICharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "../Components/DamageComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Weapons/FPSWeapon.h"

AFPSAICharacter::AFPSAICharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->SetCapsuleSize(42.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.0f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = AIStats.PatrolSpeed;

	// Create pawn sensing component
	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
	PawnSensingComponent->SetSensingInterval(0.25f);
	PawnSensingComponent->bOnlySensePlayers = false;
	PawnSensingComponent->SetSensingUpdatesEnabled(true);

	// Create damage component
	DamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("DamageComponent"));

	// Create inventory component
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// Initialize AI state
	CurrentState = EAIState::Idle;
	CombatStyle = EAICombatStyle::Tactical;
	CurrentTarget = nullptr;
	CurrentWeapon = nullptr;

	// Initialize AI stats
	AIStats.Accuracy = 0.7f;
	AIStats.ReactionTime = 0.5f;
	AIStats.SightRange = 1500.0f;
	AIStats.HearingRange = 800.0f;
	AIStats.CombatRange = 300.0f;
	AIStats.PatrolSpeed = 200.0f;
	AIStats.AlertSpeed = 400.0f;
	AIStats.CombatSpeed = 600.0f;
	AIStats.AggressionLevel = 0.5f;
	AIStats.IntelligenceLevel = 0.7f;

	// Configure pawn sensing based on AI stats
	PawnSensingComponent->SightRadius = AIStats.SightRange;
	PawnSensingComponent->HearingThreshold = AIStats.HearingRange;
	PawnSensingComponent->LOSHearingThreshold = AIStats.HearingRange * 0.5f;
	PawnSensingComponent->PeripheralVisionAngle = 60.0f;
}

void AFPSAICharacter::BeginPlay()
{
	Super::BeginPlay();

	// Bind sensing events
	if (PawnSensingComponent)
	{
		PawnSensingComponent->OnSeePawn.AddDynamic(this, &AFPSAICharacter::OnSeePlayer);
		PawnSensingComponent->OnHearNoise.AddDynamic(this, &AFPSAICharacter::OnHearNoise);
	}

	// Bind damage events
	if (DamageComponent)
	{
		DamageComponent->OnDamageTaken.AddDynamic(this, &AFPSAICharacter::OnTakeDamage);
		DamageComponent->OnDeath.AddDynamic(this, &AFPSAICharacter::OnAIDeath);
	}

	// Start with patrol if we have patrol points
	if (PatrolPoints.Num() > 0)
	{
		SetAIState(EAIState::Patrol);
	}
	else
	{
		SetAIState(EAIState::Idle);
	}
}

void AFPSAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState != EAIState::Dead)
	{
		UpdateAI(DeltaTime);
	}
}

void AFPSAICharacter::OnSeePlayer(APawn* Pawn)
{
	if (!Pawn || Pawn == this) return;

	// Check if this is a valid target
	if (Pawn->IsA<AFPSAICharacter>() && Cast<AFPSAICharacter>(Pawn)->GetCurrentState() == EAIState::Dead)
		return;

	// React based on current state
	switch (CurrentState)
	{
		case EAIState::Idle:
		case EAIState::Patrol:
			// Become alert and investigate
			CurrentTarget = Pawn;
			LastKnownTargetLocation = Pawn->GetActorLocation();
			LastTargetSeenTime = GetWorld()->GetTimeSeconds();
			SetAIState(EAIState::Alert);
			break;

		case EAIState::Alert:
		case EAIState::Searching:
		case EAIState::Investigating:
			// If we see a new target or update current target
			if (!CurrentTarget || Pawn == CurrentTarget)
			{
				CurrentTarget = Pawn;
				LastKnownTargetLocation = Pawn->GetActorLocation();
				LastTargetSeenTime = GetWorld()->GetTimeSeconds();
				
				// Escalate to combat if aggressive enough
				if (AIStats.AggressionLevel > 0.3f)
				{
					StartCombat(Pawn);
				}
			}
			break;

		case EAIState::Combat:
			// Update target if we see them
			if (Pawn == CurrentTarget)
			{
				LastKnownTargetLocation = Pawn->GetActorLocation();
				LastTargetSeenTime = GetWorld()->GetTimeSeconds();
			}
			break;
	}
}

void AFPSAICharacter::OnHearNoise(APawn* Instigator, const FVector& Location, float Volume)
{
	if (!Instigator || Instigator == this) return;

	// React based on current state and volume
	switch (CurrentState)
	{
		case EAIState::Idle:
		case EAIState::Patrol:
			if (Volume > 0.5f) // Significant noise
			{
				SetAIState(EAIState::Investigating);
				InvestigateLocation(Location);
			}
			break;

		case EAIState::Alert:
		case EAIState::Searching:
			// High priority - investigate immediately
			InvestigateLocation(Location);
			break;

		case EAIState::Combat:
			// If we lost visual contact, investigate noise
			if (!CanSeeTarget(CurrentTarget))
			{
				LastKnownTargetLocation = Location;
			}
			break;
	}
}

void AFPSAICharacter::SetAIState(EAIState NewState)
{
	if (CurrentState == NewState) return;

	EAIState PreviousState = CurrentState;
	CurrentState = NewState;

	// Handle state transitions
	switch (NewState)
	{
		case EAIState::Idle:
			StopMovement();
			break;

		case EAIState::Patrol:
			StartPatrol();
			break;

		case EAIState::Alert:
			UpdateMovementSpeed();
			break;

		case EAIState::Combat:
			UpdateMovementSpeed();
			break;

		case EAIState::Searching:
			UpdateMovementSpeed();
			break;

		case EAIState::Investigating:
			UpdateMovementSpeed();
			break;

		case EAIState::Retreating:
			FindCover();
			break;

		case EAIState::Dead:
			StopMovement();
			UnequipWeapon();
			break;
	}

	// Debug output
	if (GEngine)
	{
		FString StateText = UEnum::GetValueAsString(NewState);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
			FString::Printf(TEXT("AI State: %s"), *StateText));
	}
}

void AFPSAICharacter::UpdateAI(float DeltaTime)
{
	switch (CurrentState)
	{
		case EAIState::Idle:
			HandleIdleState(DeltaTime);
			break;

		case EAIState::Patrol:
			HandlePatrolState(DeltaTime);
			break;

		case EAIState::Alert:
			HandleAlertState(DeltaTime);
			break;

		case EAIState::Combat:
			HandleCombatState(DeltaTime);
			break;

		case EAIState::Searching:
			HandleSearchingState(DeltaTime);
			break;

		case EAIState::Investigating:
			HandleInvestigatingState(DeltaTime);
			break;

		case EAIState::Retreating:
			HandleRetreatingState(DeltaTime);
			break;
	}

	// Update movement speed based on state
	UpdateMovementSpeed();
}

void AFPSAICharacter::HandleIdleState(float DeltaTime)
{
	// Look around occasionally
	if (FMath::RandRange(0.0f, 1.0f) < 0.01f) // 1% chance per frame
	{
		FRotator NewRotation = GetActorRotation();
		NewRotation.Yaw += FMath::RandRange(-45.0f, 45.0f);
		SetActorRotation(NewRotation);
	}

	// Transition to patrol if we have patrol points
	if (PatrolPoints.Num() > 0 && FMath::RandRange(0.0f, 1.0f) < 0.005f)
	{
		SetAIState(EAIState::Patrol);
	}
}

void AFPSAICharacter::HandlePatrolState(float DeltaTime)
{
	if (PatrolPoints.Num() == 0)
	{
		SetAIState(EAIState::Idle);
		return;
	}

	// Check if we reached the current patrol point
	FVector CurrentPatrolPoint = PatrolPoints[CurrentPatrolIndex];
	float DistanceToPatrolPoint = FVector::Dist(GetActorLocation(), CurrentPatrolPoint);

	if (DistanceToPatrolPoint < 100.0f) // Within 1 meter
	{
		// Wait at patrol point
		GetWorld()->GetTimerManager().SetTimer(PatrolWaitTimer, 
			this, &AFPSAICharacter::MoveToNextPatrolPoint, PatrolWaitTime, false);
	}
	else
	{
		// Move towards patrol point
		MoveToLocation(CurrentPatrolPoint);
	}
}

void AFPSAICharacter::HandleAlertState(float DeltaTime)
{
	if (!HasValidTarget())
	{
		SetAIState(EAIState::Searching);
		return;
	}

	// Check if we can see the target
	if (CanSeeTarget(CurrentTarget))
	{
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		LastTargetSeenTime = GetWorld()->GetTimeSeconds();

		// Decide whether to engage or investigate
		float DistanceToTarget = GetDistanceToTarget(CurrentTarget);
		
		if (DistanceToTarget <= AIStats.CombatRange || AIStats.AggressionLevel > 0.7f)
		{
			StartCombat(CurrentTarget);
		}
		else
		{
			// Move closer to investigate
			MoveToLocation(LastKnownTargetLocation);
		}
	}
	else
	{
		// Move to last known location
		MoveToLocation(LastKnownTargetLocation);
		
		// If we can't see target for too long, start searching
		if (GetWorld()->GetTimeSeconds() - LastTargetSeenTime > 3.0f)
		{
			SetAIState(EAIState::Searching);
		}
	}
}

void AFPSAICharacter::HandleCombatState(float DeltaTime)
{
	if (!HasValidTarget())
	{
		SetAIState(EAIState::Searching);
		return;
	}

	UpdateCombat(DeltaTime);
}

void AFPSAICharacter::HandleSearchingState(float DeltaTime)
{
	// Search for a limited time
	if (GetWorld()->GetTimeSeconds() - LastTargetSeenTime > 15.0f)
	{
		LoseTarget();
		
		if (PatrolPoints.Num() > 0)
		{
			SetAIState(EAIState::Patrol);
		}
		else
		{
			SetAIState(EAIState::Idle);
		}
		return;
	}

	// Move to last known target location
	float DistanceToLastKnown = FVector::Dist(GetActorLocation(), LastKnownTargetLocation);
	if (DistanceToLastKnown > 50.0f)
	{
		MoveToLocation(LastKnownTargetLocation);
	}
	else
	{
		// Look around when we reach the location
		FRotator NewRotation = GetActorRotation();
		NewRotation.Yaw += FMath::RandRange(-90.0f, 90.0f) * DeltaTime;
		SetActorRotation(NewRotation);
	}
}

void AFPSAICharacter::HandleInvestigatingState(float DeltaTime)
{
	// Similar to searching but more focused
	float DistanceToInvestigation = FVector::Dist(GetActorLocation(), LastKnownTargetLocation);
	
	if (DistanceToInvestigation > 100.0f)
	{
		MoveToLocation(LastKnownTargetLocation);
	}
	else
	{
		// We've investigated enough, return to previous behavior
		if (PatrolPoints.Num() > 0)
		{
			SetAIState(EAIState::Patrol);
		}
		else
		{
			SetAIState(EAIState::Idle);
		}
	}
}

void AFPSAICharacter::HandleRetreatingState(float DeltaTime)
{
	if (bIsInCover)
	{
		// We're in cover, decide next action
		if (DamageComponent && DamageComponent->GetHealthPercentage() > 0.7f)
		{
			// Health recovered, re-engage
			SetAIState(EAIState::Combat);
		}
		else
		{
			// Stay in cover and try to heal
			// This could trigger healing items usage in the future
		}
	}
	else
	{
		// Keep moving to cover
		if (CoverLocation != FVector::ZeroVector)
		{
			MoveToLocation(CoverLocation);
			
			float DistanceToCover = FVector::Dist(GetActorLocation(), CoverLocation);
			if (DistanceToCover < 100.0f)
			{
				bIsInCover = true;
			}
		}
		else
		{
			// No cover found, try to find some
			FindCover();
		}
	}
}

void AFPSAICharacter::StartCombat(AActor* Target)
{
	CurrentTarget = Target;
	SetAIState(EAIState::Combat);
	LastKnownTargetLocation = Target->GetActorLocation();
	LastTargetSeenTime = GetWorld()->GetTimeSeconds();
}

void AFPSAICharacter::UpdateCombat(float DeltaTime)
{
	if (!HasValidTarget()) return;

	float DistanceToTarget = GetDistanceToTarget(CurrentTarget);
	bool bCanSeeCurrentTarget = CanSeeTarget(CurrentTarget);

	if (bCanSeeCurrentTarget)
	{
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		LastTargetSeenTime = GetWorld()->GetTimeSeconds();

		// Aim at target
		AimAtTarget(CurrentTarget);

		// Check if we should retreat
		if (DamageComponent && DamageComponent->GetHealthPercentage() < 0.3f && AIStats.AggressionLevel < 0.8f)
		{
			SetAIState(EAIState::Retreating);
			return;
		}

		// Combat behavior based on style
		switch (CombatStyle)
		{
			case EAICombatStyle::Aggressive:
				// Move closer and fire rapidly
				if (DistanceToTarget > AIStats.CombatRange * 0.5f)
				{
					MoveToLocation(CurrentTarget->GetActorLocation());
				}
				FireAtTarget();
				break;

			case EAICombatStyle::Defensive:
				// Keep distance and find cover
				if (DistanceToTarget < AIStats.CombatRange)
				{
					FindCover();
				}
				FireAtTarget();
				break;

			case EAICombatStyle::Tactical:
				// Balanced approach
				if (DistanceToTarget > AIStats.CombatRange * 1.5f)
				{
					MoveToLocation(CurrentTarget->GetActorLocation());
				}
				else if (DistanceToTarget < AIStats.CombatRange * 0.3f)
				{
					// Too close, back away
					FVector AwayDirection = (GetActorLocation() - CurrentTarget->GetActorLocation()).GetSafeNormal();
					MoveToLocation(GetActorLocation() + AwayDirection * 200.0f);
				}
				FireAtTarget();
				break;

			case EAICombatStyle::Sniper:
				// Stay at long range
				if (DistanceToTarget < AIStats.CombatRange * 2.0f)
				{
					FVector AwayDirection = (GetActorLocation() - CurrentTarget->GetActorLocation()).GetSafeNormal();
					MoveToLocation(GetActorLocation() + AwayDirection * 300.0f);
				}
				FireAtTarget();
				break;

			case EAICombatStyle::Rusher:
				// Always move closer
				MoveToLocation(CurrentTarget->GetActorLocation());
				FireAtTarget();
				break;
		}
	}
	else
	{
		// Lost sight of target, move to last known location
		MoveToLocation(LastKnownTargetLocation);
		
		// If we haven't seen target for too long, start searching
		if (GetWorld()->GetTimeSeconds() - LastTargetSeenTime > 5.0f)
		{
			SetAIState(EAIState::Searching);
		}
	}
}

bool AFPSAICharacter::CanSeeTarget(AActor* Target)
{
	if (!Target) return false;

	FVector StartLocation = GetActorLocation() + FVector(0, 0, 50); // Eye level
	FVector TargetLocation = Target->GetActorLocation() + FVector(0, 0, 50);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		TargetLocation,
		ECollisionChannel::ECC_Visibility,
		QueryParams
	);

	if (bHit)
	{
		return HitResult.GetActor() == Target;
	}

	return true; // No obstruction
}

bool AFPSAICharacter::IsInWeaponRange(AActor* Target)
{
	if (!Target || !CurrentWeapon) return false;

	float DistanceToTarget = GetDistanceToTarget(Target);
	return DistanceToTarget <= AIStats.CombatRange;
}

void AFPSAICharacter::AimAtTarget(AActor* Target)
{
	if (!Target) return;

	FVector TargetLocation = PredictTargetLocation(Target);
	FVector DirectionToTarget = (TargetLocation - GetActorLocation()).GetSafeNormal();
	
	FRotator TargetRotation = DirectionToTarget.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, GetWorld()->GetDeltaSeconds(), 3.0f));
}

void AFPSAICharacter::FireAtTarget()
{
	if (!CurrentWeapon || !HasValidTarget()) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	// Check fire delay based on AI reaction time and random variance
	float FireDelay = FMath::RandRange(MinFireDelay, MaxFireDelay) * (2.0f - AIStats.IntelligenceLevel);
	
	if (CurrentTime - LastFireTime > FireDelay)
	{
		// Apply accuracy - miss chance based on AI stats
		if (FMath::RandRange(0.0f, 1.0f) <= AIStats.Accuracy)
		{
			CurrentWeapon->StartFire();
			
			// Quick burst for automatic weapons
			GetWorld()->GetTimerManager().SetTimer(FireDelayTimer, [this]()
			{
				if (CurrentWeapon)
				{
					CurrentWeapon->StopFire();
				}
			}, 0.1f, false);
		}
		
		LastFireTime = CurrentTime;
	}
}

void AFPSAICharacter::FindCover()
{
	// Simple cover finding - move away from target
	if (HasValidTarget())
	{
		FVector AwayDirection = (GetActorLocation() - CurrentTarget->GetActorLocation()).GetSafeNormal();
		CoverLocation = GetActorLocation() + AwayDirection * 400.0f;
		
		// Try to find actual cover using navigation
		UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
		if (NavSystem)
		{
			FNavLocation NavLocation;
			if (NavSystem->ProjectPointToNavigation(CoverLocation, NavLocation, FVector(500, 500, 100)))
			{
				CoverLocation = NavLocation.Location;
			}
		}
	}
}

void AFPSAICharacter::MoveToCover()
{
	if (CoverLocation != FVector::ZeroVector)
	{
		MoveToLocation(CoverLocation);
	}
}

void AFPSAICharacter::MoveToLocation(FVector Location)
{
	// Use AI Controller to move
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->MoveToLocation(Location);
	}
}

void AFPSAICharacter::StopMovement()
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
}

void AFPSAICharacter::UpdateMovementSpeed()
{
	float TargetSpeed = AIStats.PatrolSpeed;

	switch (CurrentState)
	{
		case EAIState::Alert:
		case EAIState::Investigating:
		case EAIState::Searching:
			TargetSpeed = AIStats.AlertSpeed;
			break;

		case EAIState::Combat:
		case EAIState::Retreating:
			TargetSpeed = AIStats.CombatSpeed;
			break;

		case EAIState::Patrol:
		default:
			TargetSpeed = AIStats.PatrolSpeed;
			break;
	}

	GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
}

void AFPSAICharacter::StartPatrol()
{
	if (PatrolPoints.Num() > 0)
	{
		CurrentPatrolIndex = 0;
		MoveToLocation(PatrolPoints[CurrentPatrolIndex]);
	}
}

void AFPSAICharacter::MoveToNextPatrolPoint()
{
	if (PatrolPoints.Num() == 0) return;

	CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
	MoveToLocation(PatrolPoints[CurrentPatrolIndex]);
}

void AFPSAICharacter::HandlePatrolWait()
{
	// Look around while waiting
	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += FMath::RandRange(-180.0f, 180.0f);
	SetActorRotation(NewRotation);
}

void AFPSAICharacter::InvestigateLocation(FVector Location)
{
	LastKnownTargetLocation = Location;
	MoveToLocation(Location);
}

float AFPSAICharacter::GetDistanceToTarget(AActor* Target)
{
	if (!Target) return 0.0f;
	return FVector::Dist(GetActorLocation(), Target->GetActorLocation());
}

FVector AFPSAICharacter::PredictTargetLocation(AActor* Target)
{
	if (!Target) return FVector::ZeroVector;

	// Simple prediction based on target velocity
	if (APawn* TargetPawn = Cast<APawn>(Target))
	{
		FVector TargetVelocity = TargetPawn->GetVelocity();
		FVector TargetLocation = Target->GetActorLocation();
		
		// Predict where target will be based on bullet travel time
		float DistanceToTarget = GetDistanceToTarget(Target);
		float BulletSpeed = CurrentWeapon ? 800.0f : 800.0f; // Default bullet speed
		float TravelTime = DistanceToTarget / BulletSpeed;
		
		return TargetLocation + (TargetVelocity * TravelTime);
	}

	return Target->GetActorLocation();
}

bool AFPSAICharacter::HasValidTarget()
{
	return CurrentTarget && IsValid(CurrentTarget) && !CurrentTarget->IsPendingKill();
}

void AFPSAICharacter::LoseTarget()
{
	CurrentTarget = nullptr;
	LastKnownTargetLocation = FVector::ZeroVector;
	LastTargetSeenTime = 0.0f;
}

void AFPSAICharacter::EquipWeapon(TSubclassOf<AFPSWeapon> WeaponClass)
{
	if (!WeaponClass) return;

	// Unequip current weapon first
	UnequipWeapon();

	// Spawn new weapon
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	CurrentWeapon = GetWorld()->SpawnActor<AFPSWeapon>(WeaponClass, SpawnParams);
	
	if (CurrentWeapon)
	{
		// Attach weapon to character
		CurrentWeapon->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("RightHandSocket")
		);
	}
}

void AFPSAICharacter::UnequipWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
}

void AFPSAICharacter::OnTakeDamage(float Damage, EDamageType DamageType, FVector HitLocation, AActor* DamageDealer)
{
	// React to taking damage
	if (DamageDealer && DamageDealer != this)
	{
		// If we're not in combat, start combat with the attacker
		if (CurrentState != EAIState::Combat)
		{
			StartCombat(DamageDealer);
		}
		else if (DamageDealer != CurrentTarget)
		{
			// Switch targets if this is a new threat
			float DistanceToNewThreat = GetDistanceToTarget(DamageDealer);
			float DistanceToCurrentTarget = HasValidTarget() ? GetDistanceToTarget(CurrentTarget) : 10000.0f;
			
			if (DistanceToNewThreat < DistanceToCurrentTarget * 0.7f)
			{
				CurrentTarget = DamageDealer;
			}
		}

		// Alert nearby AI
		AlertToLocation(HitLocation);
	}
}

void AFPSAICharacter::OnAIDeath()
{
	SetAIState(EAIState::Dead);
	
	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Stop all timers
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AFPSAICharacter::AlertToLocation(FVector Location)
{
	LastKnownTargetLocation = Location;
	
	if (CurrentState == EAIState::Idle || CurrentState == EAIState::Patrol)
	{
		SetAIState(EAIState::Investigating);
	}
}

void AFPSAICharacter::SetCombatStyle(EAICombatStyle NewStyle)
{
	CombatStyle = NewStyle;
}

void AFPSAICharacter::AddPatrolPoint(FVector Point)
{
	PatrolPoints.Add(Point);
}

void AFPSAICharacter::SetAIStats(FAIStats NewStats)
{
	AIStats = NewStats;
	
	// Update pawn sensing component
	if (PawnSensingComponent)
	{
		PawnSensingComponent->SightRadius = AIStats.SightRange;
		PawnSensingComponent->HearingThreshold = AIStats.HearingRange;
		PawnSensingComponent->LOSHearingThreshold = AIStats.HearingRange * 0.5f;
	}
}
