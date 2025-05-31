#include "FPSAIController.h"
#include "FPSAICharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

AFPSAIController::AFPSAIController()
{
	// Create AI Perception Component
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	// Create Behavior Tree Component
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));

	// Create Blackboard Component
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

	SetupPerception();
}

void AFPSAIController::BeginPlay()
{
	Super::BeginPlay();

	SetupBehaviorTree();
}

void AFPSAIController::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	// Start behavior tree if we have one
	if (BehaviorTree && BlackboardComponent)
	{
		UseBlackboard(BehaviorTree->BlackboardAsset);
		RunBehaviorTree(BehaviorTree);
	}
}

void AFPSAIController::SetupPerception()
{
	// Setup Sight Sense
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1600.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 300.0f;
	SightConfig->DetectionByAffiliation.bNeutral = true;
	SightConfig->DetectionByAffiliation.bFriendly = true;
	SightConfig->DetectionByAffiliation.bEnemy = true;

	// Setup Hearing Sense
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 800.0f;
	HearingConfig->SetMaxAge(3.0f);
	HearingConfig->DetectionByAffiliation.bNeutral = true;
	HearingConfig->DetectionByAffiliation.bFriendly = true;
	HearingConfig->DetectionByAffiliation.bEnemy = true;

	// Setup Damage Sense
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(5.0f);

	// Add senses to perception component
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	// Set dominant sense
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	// Bind perception events
	AIPerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &AFPSAIController::OnPerceptionUpdated);
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AFPSAIController::OnTargetPerceptionUpdated);
}

void AFPSAIController::SetupBehaviorTree()
{
	// This would be set in Blueprint or loaded from asset
	// For now, we'll handle AI logic in the character class
}

void AFPSAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	for (AActor* Actor : UpdatedActors)
	{
		if (Actor && Actor != GetPawn())
		{
			FAIStimulus Stimulus;
			if (AIPerceptionComponent->GetActorsPerception(Actor, Stimulus))
			{
				OnTargetPerceptionUpdated(Actor, Stimulus);
			}
		}
	}
}

void AFPSAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || Actor == GetPawn()) return;

	AFPSAICharacter* AICharacter = Cast<AFPSAICharacter>(GetPawn());
	if (!AICharacter) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		// We can sense this actor
		switch (Stimulus.Type.Index)
		{
			case 0: // Sight
				{
					// Handle sight stimulus
					SetTargetActor(Actor);
					SetLastKnownLocation(Actor->GetActorLocation());
					
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, 
							FString::Printf(TEXT("AI saw: %s"), *Actor->GetName()));
					}
				}
				break;

			case 1: // Hearing
				{
					// Handle hearing stimulus
					SetLastKnownLocation(Stimulus.StimulusLocation);
					
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
							FString::Printf(TEXT("AI heard: %s"), *Actor->GetName()));
					}
				}
				break;

			case 2: // Damage
				{
					// Handle damage stimulus
					SetTargetActor(Actor);
					SetLastKnownLocation(Actor->GetActorLocation());
					
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, 
							FString::Printf(TEXT("AI damaged by: %s"), *Actor->GetName()));
					}
				}
				break;
		}
	}
	else
	{
		// Lost sight of actor
		if (GetTargetActor() == Actor)
		{
			// Keep last known location but clear direct reference
			// Don't clear target immediately - let the AI search
		}
	}
}

void AFPSAIController::SetTargetActor(AActor* Target)
{
	if (BlackboardComponent)
	{
		BlackboardComponent->SetValueAsObject(TargetActorKey, Target);
	}
}

void AFPSAIController::SetLastKnownLocation(FVector Location)
{
	if (BlackboardComponent)
	{
		BlackboardComponent->SetValueAsVector(LastKnownLocationKey, Location);
	}
}

void AFPSAIController::SetAIState(int32 State)
{
	if (BlackboardComponent)
	{
		BlackboardComponent->SetValueAsInt(AIStateKey, State);
	}
}

void AFPSAIController::ClearTarget()
{
	if (BlackboardComponent)
	{
		BlackboardComponent->ClearValue(TargetActorKey);
		BlackboardComponent->ClearValue(LastKnownLocationKey);
	}
}

AActor* AFPSAIController::GetTargetActor() const
{
	if (BlackboardComponent)
	{
		return Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorKey));
	}
	return nullptr;
}

FVector AFPSAIController::GetLastKnownLocation() const
{
	if (BlackboardComponent)
	{
		return BlackboardComponent->GetValueAsVector(LastKnownLocationKey);
	}
	return FVector::ZeroVector;
}

int32 AFPSAIController::GetAIState() const
{
	if (BlackboardComponent)
	{
		return BlackboardComponent->GetValueAsInt(AIStateKey);
	}
	return 0;
}

bool AFPSAIController::CanSeeActor(AActor* Actor) const
{
	if (!Actor || !AIPerceptionComponent) return false;

	TArray<AActor*> PerceivedActors;
	AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	return PerceivedActors.Contains(Actor);
}

float AFPSAIController::GetDistanceToActor(AActor* Actor) const
{
	if (!Actor || !GetPawn()) return 0.0f;

	return FVector::Dist(GetPawn()->GetActorLocation(), Actor->GetActorLocation());
}

void AFPSAIController::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	if (GetPawn())
	{
		OutLocation = GetPawn()->GetActorLocation() + FVector(0, 0, 60); // Eye level
		OutRotation = GetPawn()->GetActorRotation();
	}
	else
	{
		Super::GetActorEyesViewPoint(OutLocation, OutRotation);
	}
}
